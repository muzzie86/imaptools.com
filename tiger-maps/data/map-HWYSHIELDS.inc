    # map-HWYSHIELDS.inc
    LAYER
        NAME "hwyshieldsnear"
        METADATA
            "ows_title"     "Hwy Shields Near"
            "ows_srs"       "EPSG:4269 EPSG:4326"
            "ows_abstract"  "Hwy Shields for the U.S. Census ROADS data layer."
        END
        TYPE POINT
        TILEINDEX "tidx-ROADS"
        PROJECTION
            "+proj=longlat +ellps=GRS80 +datum=NAD83" #EPSG: 4269
        END
        MAXSCALEDENOM  300000
        FILTER (length('[RTENUM]')>0 && '[RTTYP]' ~ '^(I|U|S)$')
        CLASSITEM "RTTYP"
        LABELITEM "RTENUM"
        CLASS
          EXPRESSION "I"
          LABEL
            STYLE
              SYMBOL "I"
            END
            SIZE TINY
            POSITION CC
            OFFSET 1 0
            MINDISTANCE 100
            COLOR "#ffffff"
            PARTIALS FALSE
            PRIORITY 7
          END
        END
        CLASS
          EXPRESSION "U"
          LABEL
            STYLE
              SYMBOL "U"
            END
            SIZE TINY
            POSITION CC
            OFFSET 1 0
            MINDISTANCE 100
            COLOR "#000000"
            PARTIALS FALSE
            PRIORITY 7
          END
        END
        CLASS
          EXPRESSION "S"
          LABEL
            STYLE
              SYMBOL "circle-filled"
              COLOR "#FFFFFF"
              OUTLINECOLOR "#000000"
              SIZE 19
            END
            SIZE TINY
            POSITION CC
            MINDISTANCE 100
            COLOR "#000000"
            PARTIALS FALSE
            PRIORITY 7
          END
        END
    END

