import re
import cartopy.crs as ccrs
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import cartopy.feature as cfeature
import cartopy.io.shapereader as shapereader
import cartopy.geodesic as cgeodesic
import shapely.geometry as sgeometry
import shapely.ops as sops
import numpy as np
from scipy.interpolate import interp1d
from matplotlib import patches

class Ploter:
    def __init__(self, filename):
        self.filename = filename
        self.file = open(self.filename)
        self.content = self.file.read()

    def run(self):
        self.analyze_content()
        self.init_plot()
        self.interpolate()
        self.make_cone()
        self.draw()

    def analyze_content(self):
        with open(self.filename) as fp:
            for i, line in enumerate(fp):
                if (i == 2):
                    self.basicinfo = line.split(' ')
                if (i == 5):
                    self.issuetime = line.split(' ')[-1]
                if (i == 10):
                    self.initpres = line.split(' ')[-2]
                    break
        pos = re.findall(r'\d{1,2}\.\d[NS] \d{1,3}\.\d[EW]', self.content)
        self.initpos = pos[0]
        self.lats = []
        self.lons = []
        
        for p in pos:
            latstr, lonstr = tuple(p.split())
            lat = float(latstr[:-1])
            if latstr[-1] == 'S':
                lat = -lat
            lon = float(lonstr[:-1])
            if lonstr[-1] == 'W':
                lon = 360 - lon
            self.lats.append(lat)
            self.lons.append(lon)
        wnds = re.findall(r'WINDS? (\d{1,3}) KT', self.content)
        self.winds = list(map(int, wnds))
        tmes = re.findall(r'P\+(\d{1,3})HR', self.content)
        self.times = list(map(int, tmes))
        self.times.insert(0, 0)

        # self.times = [    0,    12,    24,    36,    48,    72,    96,   120]
        # self.lats  = [ 22.1,  21.8,  21.4,  21.6,  22.3,  23.2,  24.5,  28.1]
        # self.lons  = [144.9, 144.1, 142.9, 141.5, 140.0, 138.1, 136.2, 133.8]
        # self.winds = [   35,    45,    55,    65,    75,    95,   110,   115]

        ne = re.findall(r'(\d+)%s' % 'NE', self.content)
        se = re.findall(r'(\d+)%s' % 'SE', self.content)
        sw = re.findall(r'(\d+)%s' % 'SW', self.content)
        nw = re.findall(r'(\d+)%s' % 'NW', self.content)
        # wind quad in NE, SE, SW, NW
        self.q34kt = list(map(int, [ne[0], se[0], sw[0], nw[0]]))
        self.q50kt = list(map(int, [ne[1], se[1], sw[1], nw[1]]))
        self.q64kt = list(map(int, [ne[2], se[2], sw[2], nw[2]]))

        # errs in meters
        #     hrs =  0,    12,    24,     36,     48,     72,     96,    120
        self.errs = [0, 56529, 89490, 122451, 155412, 236818, 338263, 456217]
        # the following includes TAU+60 err
        # [56529, 89490, 122451, 155412, 196115, 236818, 338263, 456217]

    def readShpFile(self, path, ec, fc, lw):
        reader = shapereader.Reader(path)
        shape_feature = cfeature.ShapelyFeature(reader.geometries(), \
            ccrs.PlateCarree(), edgecolor=ec, facecolor=fc, linewidth=lw)
        self.ax.add_feature(shape_feature)
    
    def init_plot(self):
        #maplimits = [105, 152, 16, 46]  # [Wlon, Elon, Slat, Nlat]
        self.ax = plt.axes(projection=ccrs.Mercator(central_longitude=180))
        #self.ax.set_extent(maplimits, ccrs.PlateCarree())

        self.readShpFile('./shapefiles/ne_10m_ocean/ne_10m_ocean', 'none', \
            [0.476, 0.661, 0.816, 1], 0)
        self.readShpFile('./shapefiles/ne_10m_admin_0_countries/ne_10m_admin_0_countries', \
            'black', [0.753, 0.753, 0.753, 1], 0.75)
        self.readShpFile('./shapefiles/province/province', 'black', 'none', 0.5)
        self.readShpFile('./shapefiles/ne_50m_admin_1_states_provinces/' \
            'ne_50m_admin_1_states_provinces', 'black', 'none', 0.5)
        self.readShpFile('./shapefiles/ne_10m_lakes/ne_10m_lakes', 'black', \
            [0.476, 0.661, 0.816, 1], 0.5)

        gl = self.ax.gridlines(draw_labels=True, color=[0.437, 0.512, 0.565, 1], \
            linestyle='--', zorder=2)
        gl.xlocator = gl.ylocator = mticker.MultipleLocator(5)
        gl.top_labels = False
    
    def interpolate(self):
        idx = len(self.lats) - 1
        num_of_points = self.times[idx] // 3 + 1  # interpolate interval: 3 hrs
        self.interp_times = np.linspace(self.times[0], self.times[idx], \
            num_of_points)
        self.interp_lats = interp1d(self.times, self.lats, kind='cubic') \
            (self.interp_times)
        self.interp_lons = interp1d(self.times, self.lons, kind='cubic') \
            (self.interp_times)
        self.interp_errs = interp1d(self.times, self.errs[0:idx+1], \
            kind='linear')(self.interp_times)
    
    def makeErrCircle(self, lat, lon, radius):
        circle_points = cgeodesic.Geodesic().circle(lon=lon, lat=lat, \
            radius=radius, n_samples=100, endpoint=False)
        return sgeometry.Polygon(circle_points)
    
    def make_cone(self):
        polygons = [self.makeErrCircle(lat, lon, radius) \
            for lat, lon, radius in zip(self.interp_lats, self.interp_lons, \
                self.interp_errs)]
        convex_hulls = [sgeometry.MultiPolygon([polygons[i], polygons[i+1]]).convex_hull \
            for i in range(len(polygons)-1)]
        self.cone = sops.cascaded_union(convex_hulls)

    def getIntensity(self, wind):
        if wind < 34:
            return '$D$'
        elif wind < 64:
            return '$S$'
        elif wind < 83:
            return '$1$'
        elif wind < 96:
            return '$2$'
        elif wind < 113:
            return '$3$'
        elif wind < 137:
            return '$4$'
        else:
            return '$5$'
    
    def getArc(self, lat, lon, radius, start, end):
        lat = np.deg2rad(lat)
        lon = np.deg2rad(lon)
        # earth radius
        # R = 6378.1Km
        # R = ~ 3959 MilesR = 3959
        R = 3440.06479482	# NM

        radius /= R

        brng = np.deg2rad(np.arange(start, end+1))

        lats = np.arcsin(np.sin(lat) * np.cos(radius) +
            np.cos(lat) * np.sin(radius) * np.cos(brng))
        lons = lon + np.arctan2(np.sin(brng) * np.sin(radius) * \
            np.cos(lat), np.cos(radius) - np.sin(lat) * np.sin(lats))

        # lat2 = math.asin(math.sin(lat1) * math.cos(radius)
        #     + math.cos(lat1) * math.sin(radius) * math.cos(brng))

        # lon2 = lon1 + math.atan2(math.sin(brng) * math.sin(radius)
        #     * math.cos(lat1), math.cos(radius)-math.sin(lat1)*math.sin(lat2))

        lats = np.rad2deg(lats)
        lons = np.rad2deg(lons)

        return lons, lats

    def getArcArray(self, theta, rad):
        self.posArray = [(self.lons[0], self.lats[0])]
        lons, lats = self.getArc(self.lats[0], self.lons[0], rad, theta, \
            theta+91)
        coords = list(zip(lons, lats))
        self.posArray += coords

    def drawEachWindQuad(self, windq, fc):
        polygons = []
        for rad, theta in zip(windq, [0, 90, 180, 270]):
            if (rad > 0):
                self.getArcArray(theta, rad)
                polygon = sgeometry.Polygon(self.posArray)
                polygons.append(polygon)
        wedge = sops.cascaded_union(polygons)
        self.ax.add_geometries((wedge,), crs=ccrs.PlateCarree(), facecolor=fc)
        return wedge
    
    def drawWindQuad(self):
        self.wedge34kt = self.drawEachWindQuad(self.q34kt, [0, 1, 0, 0.3])
        self.drawEachWindQuad(self.q50kt, [1, 1, 0, 0.3])
        self.drawEachWindQuad(self.q64kt, [1, 0, 0, 0.3])
    
    def geoscale(self, latmin, latmax, lonmin, lonmax, pad, scale):
        latmid = (latmax + latmin) / 2
        dlat = latmax - latmin + pad * 2
        lonmid = (lonmax + lonmin) / 2
        dlon = lonmax - lonmin + pad * 2
        if dlon / dlat < scale:
            dlon = dlat * scale
        else:
            dlat = dlon / scale
        latmin = latmid - dlat / 2
        latmax = latmid + dlat / 2
        lonmin = lonmid - dlon / 2
        lonmax = lonmid + dlon / 2
        return latmin, latmax, lonmin, lonmax

    def defineExtend(self):
        temp = [self.wedge34kt, self.cone]
        wholewedge = sops.cascaded_union(temp)
        excoords = wholewedge.exterior.coords
        olons, olats = list(zip(*excoords))
        lonmin, lonmax = np.amin(olons), np.amax(olons)
        latmin, latmax = np.amin(olats), np.amax(olats)
        latmin, latmax, lonmin, lonmax = self.geoscale(latmin, latmax,
        lonmin, lonmax, pad=1, scale=1.7)
        extend = [lonmin, lonmax, latmin, latmax]
        self.ax.set_extent(extend, ccrs.PlateCarree())
    
    def drawTitle(self):
        dot = u" \u2022 "
        if (self.basicinfo[3] == 'FORECAST/ADVISORY'):
            title = self.basicinfo[0].title() + ' ' + self.basicinfo[1].title()\
                + ' ' + self.basicinfo[2] + ' Forecast Track'
        else:
            title = self.basicinfo[0].title() + ' ' + self.basicinfo[1] + \
                ' Forecast Track'
        small_title = self.initpos + dot + str(self.winds[0]) + ' kt' + dot \
            + self.initpres + ' hPa\nHCXA issued at: ' + self.issuetime[:-1]
        self.ax.set_title(title, loc='left', fontsize=17, fontweight='bold')
        self.ax.set_title(small_title, loc='right', fontsize=12)

    def drawWarnTxt(self):
        text = "This is not an official forecast and should not be used as " \
            "such.\nFor official information, please refer to products from " \
            "your RSMC."
        self.ax.text(0.01, 0.01, text, fontsize=9, color='red',
            transform=self.ax.transAxes, ha='left', va='bottom', zorder=7)

    def draw(self):
        self.drawWindQuad()
        self.ax.plot(self.interp_lons, self.interp_lats, linewidth=1.75, \
            linestyle='--', color='white', transform=ccrs.PlateCarree())
        self.ax.add_geometries((self.cone,), crs=ccrs.PlateCarree(), \
            facecolor=[1, 1, 1, 0.2], edgecolor='white', linewidth=2.5, zorder=3)
        self.ax.scatter(self.lons[1:], self.lats[1:], s=150, facecolor='white', 
            transform=ccrs.PlateCarree(), zorder=4)
        for i in range(len(self.winds)-1):
            self.ax.scatter(self.lons[i+1], self.lats[i+1], \
                marker=self.getIntensity(self.winds[i+1]), color='black', \
                transform=ccrs.PlateCarree(), zorder=5)
        self.ax.plot(self.lons[0], self.lats[0], marker='x', markersize=8, \
            color='black', transform=ccrs.PlateCarree(), zorder=6)
        self.drawTitle()
        self.drawWarnTxt()
        self.defineExtend()
        plt.show()

    def close(self):
        self.file.close()

Ploter('wtwp20.hcxa..txt').run()