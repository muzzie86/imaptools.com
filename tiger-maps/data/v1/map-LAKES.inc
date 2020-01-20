  # map-LAKES.inc
  LAYER
    NAME "Lakes Z0"
    GROUP "cb"
    STATUS DEFAULT
    TYPE POLYGON
    DATA "not-nt2/110m-lakes"
    PROJECTION "init=epsg:4326" END
    MINSCALEDENOM 100000000
    CLASS
      STYLE
        COLOR "#a5bfdd"
      END
    END
  END
  LAYER
    NAME "Lakes Z1"
    GROUP "cb"
    STATUS DEFAULT
    TYPE POLYGON
    DATA "not-nt2/50m-lakes"
    PROJECTION "init=epsg:4326" END
    MAXSCALEDENOM 100000000
    MINSCALEDENOM  20000000
    CLASS
      STYLE
        COLOR "#a5bfdd"
      END
    END
  END
  LAYER
    NAME "Lakes Z2"
    GROUP "cb"
    STATUS DEFAULT
    TYPE POLYGON
    DATA "not-nt2/10m-lakes"
    PROJECTION "init=epsg:4326" END
    MAXSCALEDENOM  20000000
    #MINSCALEDENOM   1000000
    CLASS
      STYLE
        COLOR "#a5bfdd"
      END
    END
  END

