    LAYER
        NAME "hwyshieldsfar"
        METADATA
            "ows_title"     "Hwy Shields Far"
            "ows_srs"       "EPSG:4269 EPSG:4326"
            "ows_abstract"  "Hwy Shields for the U.S. Census ROADS data layer."
        END
        TYPE ANNOTATION
        TILEINDEX "tidx-PRISECROADS-S"
        PROJECTION
            "+proj=longlat +ellps=GRS80 +datum=NAD83" #EPSG: 4269
        END
        MAXSCALEDENOM 5000000
        MINSCALEDENOM  300000
        FILTER ('[MTFCC]'='S1100' && length('[RTENUM]')>0 && ('[RTTYP]'='I' || '[RTTYP]'='U'))
        CLASSITEM "RTTYP"
        LABELITEM "RTENUM"
        CLASS
          EXPRESSION "I"
          STYLE
            SYMBOL "I"
          END
          LABEL
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
          STYLE
            SYMBOL "U"
          END
          LABEL
            SIZE TINY
            POSITION CC
            OFFSET 1 0
            MINDISTANCE 100
            COLOR "#000000"
            PARTIALS FALSE
            PRIORITY 7
          END
        END
    END

    LAYER
        NAME "hwyshieldsnear"
        METADATA
            "ows_title"     "Hwy Shields Near"
            "ows_srs"       "EPSG:4269 EPSG:4326"
            "ows_abstract"  "Hwy Shields for the U.S. Census ROADS data layer."
        END
        TYPE ANNOTATION
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
          STYLE
            SYMBOL "I"
          END
          LABEL
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
          STYLE
            SYMBOL "U"
          END
          LABEL
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
          STYLE
            SYMBOL "circle-filled"
            COLOR "#FFFFFF"
            OUTLINECOLOR "#000000"
            SIZE 19
          END
          LABEL
            SIZE TINY
            POSITION CC
            MINDISTANCE 100
            COLOR "#000000"
            PARTIALS FALSE
            PRIORITY 7
          END
        END
    END

