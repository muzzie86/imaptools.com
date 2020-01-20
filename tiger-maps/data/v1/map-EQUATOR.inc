  # map-EQUATOR.inc
  LAYER
    NAME "Equator"
    GROUP "cd"
    STATUS DEFAULT
    TYPE LINE
    PROJECTION "init=epsg:4326" END
    MINSCALEDENOM 50000000
    FEATURE
      POINTS -180 0 180 0 END
    END
    CLASS
      STYLE
        COLOR "#212121"
        PATTERN 3 5 END
      END
    END
  END
