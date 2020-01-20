  # map-WORLDNAMES.inc
  LAYER
    NAME "World Names Z0"
    GROUP "cb"
    STATUS DEFAULT
    TYPE POINT
    DATA "not-nt2/world/world-names"
    PROJECTION "init=epsg:4326" END
    LABELITEM "NAME"
    CLASS
      MINSCALEDENOM 200000000
      LABEL
        ENCODING "UTF8"
        TYPE TRUETYPE FONT "arial" SIZE 10 COLOR "#212121"
        PARTIALS FALSE OUTLINECOLOR "#ffffff" OUTLINEWIDTH 3
        BUFFER 4 POSITION UC MINDISTANCE 250 MINFEATURESIZE 10
        PRIORITY 4 WRAP " " ALIGN CENTER
      END
    END
    CLASS
      MAXSCALEDENOM 200000000
      MINSCALEDENOM   4000000
      LABEL
        ENCODING "UTF8"
        TYPE TRUETYPE FONT "arial" SIZE 12 COLOR "#212121"
        PARTIALS FALSE OUTLINECOLOR "#ffffff" OUTLINEWIDTH 3
        BUFFER 4 POSITION UC MINDISTANCE 250 MINFEATURESIZE 10
        PRIORITY 4 WRAP " " ALIGN CENTER
      END
    END
  END

  LAYER
    NAME "World Names Z1"
    GROUP "cb"
    STATUS DEFAULT
    TYPE POINT
    DATA "not-nt2/110m-populated-places"
    PROJECTION "init=epsg:4326" END
    FILTERITEM "FEATURECLA"
    FILTER /^Admin-0 capital$/
    LABELITEM "NAME_TOWN"
    CLASS
      MAXSCALEDENOM  50000000
      MINSCALEDENOM  20000000
      LABEL
        STYLE
          SYMBOL "city_3"
        END
        ENCODING "UTF8"
        TYPE TRUETYPE FONT "arial" SIZE 8 COLOR "#291509"
        PARTIALS FALSE OUTLINECOLOR "#ffffff" OUTLINEWIDTH 1
        BUFFER 4 POSITION UC PRIORITY 5 
      END
    END
  END

  LAYER
    NAME "World Names Z2"
    GROUP "cb"
    STATUS DEFAULT
    TYPE POINT
    DATA "not-nt2/10m-populated-places"
    PROJECTION "init=epsg:4326" END
    CLASSITEM "FEATURECLA"
    LABELITEM "NAME_TOWN"
    MAXSCALEDENOM  20000000
    MINSCALEDENOM   4000000
    CLASS
      EXPRESSION /^Admin-0 capital$/
      LABEL
        STYLE
          SYMBOL "city_2"
        END
        ENCODING "UTF8"
        TYPE TRUETYPE FONT "arial" SIZE 8 COLOR "#291509"
        PARTIALS FALSE OUTLINECOLOR "#ffffff" OUTLINEWIDTH 1
        BUFFER 4 POSITION UC PRIORITY 5 
      END
    END
    CLASS
      EXPRESSION /^\s*$/
      LABEL
        STYLE
          SYMBOL "city_5"
        END
        ENCODING "UTF8"
        TYPE TRUETYPE FONT "arial" SIZE 8 COLOR "#5D4D45"
        PARTIALS FALSE OUTLINECOLOR "#ffffff" OUTLINEWIDTH 1
        BUFFER 6 POSITION UC PRIORITY 3 
      END
    END
    CLASS
      LABEL
        STYLE
          SYMBOL "city_4"
        END
        ENCODING "UTF8"
        TYPE TRUETYPE FONT "arial" SIZE 8 COLOR "#5D4D45"
        PARTIALS FALSE OUTLINECOLOR "#ffffff" OUTLINEWIDTH 1
        BUFFER 4 POSITION UC PRIORITY 4
      END
    END
  END

  LAYER
    NAME "US_Places"
    STATUS DEFAULT
    DATA "not-nt2/us/placenames2"
    TYPE POINT
    FILTER ([POP] >= 5000)
    CLASSITEM "POP"
    MAXSCALEDENOM 4000000
    MINSCALEDENOM 1000000
    LABELITEM "NAME"
    CLASS
      NAME "Mega Cities"
      EXPRESSION ([POP] > 100000)
      LABEL
        STYLE
          SYMBOL "city_2"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 5
        POSITION AUTO
        BUFFER 5
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS
      NAME "Major Cities"
      EXPRESSION ([POP] >= 25000 && [POP] < 99999)
      LABEL
        STYLE
          SYMBOL "city_3"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 7
        PRIORITY 4
        POSITION AUTO
        BUFFER 5
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS
      NAME "Cities"
      EXPRESSION ([POP] >= 5000 && [POP] < 24999)
      LABEL
        STYLE
          SYMBOL "city_5"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 7
        PRIORITY 3
        POSITION AUTO
        BUFFER 5
        COLOR "#5D4D45"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
  END

  LAYER
    NAME "US_State_Capitals"
    STATUS DEFAULT
    DATA "not-nt2/us/state_caps"
    TYPE POINT
    MAXSCALEDENOM 4000000
    MINSCALEDENOM 1000000
    LABELITEM "NAME"
    CLASS
      NAME "State Capitals"
      LABEL
        STYLE
          SYMBOL "city_2"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 5
        POSITION AUTO
        BUFFER 5
        COLOR 51 51 0
        OUTLINECOLOR 230 230 230
        PARTIALS FALSE
      END
    END
  END


