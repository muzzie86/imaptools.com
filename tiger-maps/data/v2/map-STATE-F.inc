  # map-STATE.inc
  CLASS
    MAXSCALEDENOM 100000000
    MINSCALEDENOM  50000000
    TEXT ('[STUSPS]')
    STYLE
      COLOR "#f2efe9"
    END
    LEADER
      MAXDISTANCE 30
      GRIDSTEP 5
      STYLE
        GEOMTRANSFORM 'end'
        SYMBOL 'circle-filled'
        SIZE 6.5
        COLOR "#000000"
      END
      STYLE
        COLOR "#000000"
        WIDTH 0.5
      END
    END
    LABEL
      COLOR "#675844"
      FONT "arial"
      TYPE TRUETYPE
      SIZE 6
      POSITION CC
      PARTIALS FALSE
      BUFFER 1
      MINDISTANCE 200
      OUTLINECOLOR "#ffffff"
      OUTLINEWIDTH 1
    END
  END
  CLASS
    MAXSCALEDENOM  50000000
    MINSCALEDENOM  20000000
    TEXT ('[NAME]')
    STYLE
      COLOR "#f2efe9"
    END
    LEADER
      MAXDISTANCE 30
      GRIDSTEP 5
      STYLE
        COLOR "#000000"
        WIDTH 1.0
      END
    END
    LABEL
      COLOR "#675844"
      FONT "sans-bold"
      TYPE TRUETYPE
      SIZE 7
      POSITION CC
      PARTIALS FALSE
      WRAP ' '
      ALIGN CENTER
      BUFFER 2
      MINDISTANCE 200
      OUTLINECOLOR "#ffffff"
      OUTLINEWIDTH 1
    END
  END
  CLASS
    MAXSCALEDENOM  20000000
    MINSCALEDENOM   5000000
    TEXT ('[NAME]')
    STYLE
      COLOR "#f2efe9"
    END
    LEADER
      MAXDISTANCE 30
      GRIDSTEP 5
      STYLE
        COLOR "#000000"
        WIDTH 1.0
      END
    END
    LABEL
      COLOR "#675844"
      FONT "sans-bold"
      TYPE TRUETYPE
      SIZE 8
      POSITION CC
      PARTIALS FALSE
      WRAP ' '
      ALIGN CENTER
      BUFFER 2
      MINDISTANCE 200
      OUTLINECOLOR "#ffffff"
      OUTLINEWIDTH 1
    END
  END
