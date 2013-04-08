#!/usr/bin/env python
#coding=utf-8

import math

class plotMarker:

    # rectSize is given in pixels.
    def __init__(self, c_width, c_height, plid=0, rectSize=36):
        if ( c_width < 4 or c_height < 4 ):
            raise PM_AxisTooSmallException()

        # Do I have enough bits for the plid?
        power = (math.floor( ((c_width-1) * (c_height -1))/2 ) - 7) * 2
        if ( power < 1 or pow(2,power) < plid ):
            raise PM_InsuficientBitsException(plid)

        self.plid = plid
        self.c_width = c_width
        self.c_height = c_height
        self.ss = rectSize
        self.svgWidth = (5+self.c_width)*self.ss
        self.svgHeight = (6+self.c_height)*self.ss # 6 for the ID text
        self.Brects = [] # Black rectangles
        self.Crects = [] # Color rectangles
        self.margins = []

        self.initMargin()
        self.initBlack()
        self.initColor()

        self.svgheader = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
"""
        self.svgstart = """
<svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   width="%s"
   height="%s"
   sodipodi:docname="chessboard.svg">
""" % (self.svgWidth, self.svgHeight)
        self.svgend = """</svg>"""
        self.sodipodiheader = """
  <sodipodi:namedview
     id="base"
     pagecolor="#ffffff"
     bordercolor="#666666"
     borderopacity="1.0"
     inkscape:pageopacity="0.0"
     inkscape:pageshadow="2"
     inkscape:document-units="mm"
     inkscape:current-layer="layer1"
     units="mm"
     inkscape:window-maximized="1">
  </sodipodi:namedview>
"""
        self.metadata = """
  <metadata
     id="metadata7">
    <rdf:RDF>
      <cc:Work
         rdf:about="">
        <dc:format>image/svg+xml</dc:format>
        <dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" />
        <dc:title />
      </cc:Work>
    </rdf:RDF>
  </metadata>
"""
        self.idText = """
  <text
     xml:space="preserve"
     style="font-size:16px;font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;text-align:center;line-height:125%%;letter-spacing:0px;word-spacing:0px;text-anchor:middle;fill:#000000;fill-opacity:1;stroke:none;font-family:Symbol;-inkscape-font-specification:Symbol"
     x="%d"
     y="%d"
     id="text3018"
     sodipodi:linespacing="125%%"><tspan
       sodipodi:role="line"
       id="tspan3020"
       x="%d"
       y="%d">ID:%d</tspan>
  </text>
""" % (20, (6+self.c_height)*self.ss, 20, (6+self.c_height)*self.ss, self.plid)


    def __str__(self):
        svgStr = self.svgheader + self.svgstart \
                    + self.sodipodiheader + self.metadata
        for margin in self.margins:
            svgStr = svgStr + str(margin)
        for rect in self.Brects:
            svgStr = svgStr + str(rect)
        for rect in self.Crects:
            svgStr = svgStr + str(rect)
        svgStr = svgStr + self.idText
        svgStr = svgStr + self.svgend
        return (svgStr)

    def initMargin (self):
        # bottom margin
        self.margins.append( Rectangle("#000000", "topM",
            0, 0, (5+self.c_width)*self.ss, self.ss) )
        # top margin
        self.margins.append( Rectangle("#000000", "bottomM",
            0, (4+self.c_height)*self.ss, (5+self.c_width)*self.ss, self.ss) )
        # left margin
        self.margins.append( Rectangle("#000000", "leftM",
            0, self.ss, self.ss, (3+self.c_height)*self.ss) )
        # right margin
        self.margins.append( Rectangle("#000000", "rightM",
            (4+self.c_width)*self.ss, self.ss, self.ss, (3+self.c_height)*self.ss) )

    def initBlack (self):
        # Create iner squares
        for sq in range( (self.c_width+1) * (self.c_height+1) ):
            V = math.floor(float(sq)/(self.c_width+1))
            # Start from left when even. Start from right when uneven.
            H = abs((sq % (self.c_width+1)) - ((V%2)*self.c_width))
            y = (V+2)*self.ss
            x = (H+2)*self.ss
            if (sq%2) == 0:
                self.Brects.append ( Rectangle("#000000", "B%s"%sq,
                    x, y, self.ss, self.ss) )

    def initColor (self):
        for v in range(self.c_height-1):
            for h in range(self.c_width-1):
                x = (h+3)*self.ss
                y = (v+3)*self.ss
                if (v%2) == abs((h%2)-1):
                    self.Crects.append ( Rectangle("#ff0000",
                        "C(%s,%s)"%(v,h), x, y, self.ss, self.ss,
                        opacity="0.5") )

        cols = ["#ff0000", "#ffff00", "#00ff00",
                "#00ffff", "#0000ff", "#ff00ff"]
        for i in range(6):
            self.Crects[i].color = cols[i]
            self.Crects[i].opacity = "0.5"
        self.Crects[6]._color = "#ffffff"
        self.Crects[6].opacity = "1"

        # format the binary representation of self.plid
        binPlid = bin(self.plid)[2:][::-1] #reverse the bin representation
        for i in range(len(binPlid)):
            ro = int(math.floor(i/2) + 7) # rectangle offset
            if i%2 == 0: # even is green
                self.Crects[ro].green = int(binPlid[i])
            else: # uneven is blue
                self.Crects[ro].blue = int(binPlid[i])

class Rectangle(object):
    def __init__(self, color, id, x, y, width, height, opacity="1"):
        self._color = color
        self.id = id
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self._opacity = opacity
        self.style = "fill:%s;fill-opacity:%s;stroke:None" \
                % (self._color, self._opacity)
    @property
    def color(self):
        return (self._color)

    @color.setter
    def color(self, value):
        self._color = value
        if value == "#ffffff":
            self._color = "#00ff00" # we replace white with green
        self.style = "fill:%s;fill-opacity:%s;stroke:None" \
                % (self._color, self._opacity)

    @property
    def opacity(self):
        return (self._opacity)

    @opacity.setter
    def opacity(self, value):
        self._opacity = value
        self.style = "fill:%s;fill-opacity:%s;stroke:None" \
                % (self._color, self._opacity)

    @property
    def red(self):
        return (self._color[1:3])

    @red.setter
    def red(self, value):
        r = "ff"
        if value <= 0:
            r="00"
        self.color = "#" + r + self._color[3:]

    @property
    def blue(self):
        return (self._color[3:5])

    @blue.setter
    def blue(self, value):
        b = "00"
        if value > 0:
            b = "ff"
        self.color = self._color[:5] + b

    @property
    def green(self):
        return (self._color[5:])

    @green.setter
    def green(self, value):
        g = "00"
        if value > 0:
            g = "ff"
        self.color = self._color[:3] + g + self._color[5:]

    def __str__(self):
        return ("""
    <rect
       style="%s"
       id="%s"
       width="%s"
       height="%s"
       x="%s"
       y="%s" />
""" % (self.style, self.id, self.width, self.height, self.x, self.y))

class PMException(Exception):
    def __init__(self):
        pass
    def __str__(self):
        return ("PM_ERROR: %s" % self.message)
class PM_InsuficientBitsException(PMException):
    def __init__(self, v):
        self.message = "There are not enough bits for this int %d."%v
class PM_AxisTooSmallException(PMException):
    def __init__(self):
        self.message = "Axis of the chessboard are too small."
