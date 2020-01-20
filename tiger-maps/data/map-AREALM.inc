  # map-AREALM.inc
  MAXSCALEDENOM 2500000
  CLASSITEM "MTFCC"
  LABELITEM "FULLNAME"
  CLASS
    EXPRESSION "C3023" # Islands
    STYLE
      COLOR "#F4F3F0"
    END
    LABEL
      TYPE TRUETYPE
      FONT "sans"
      SIZE 8
      PRIORITY 5
      POSITION CC
      BUFFER 5
      COLOR "#291509"
      OUTLINECOLOR "#ffffff"
      WRAP " "
      ALIGN CENTER
      MAXLENGTH 7
      PARTIALS FALSE
    END
  END
  CLASS
    EXPRESSION "K1231" # Hospital
    STYLE
      COLOR "#EBD2CF"
    END
    LABEL
      STYLE
        SYMBOL "hospital"
        SIZE 16
      END
      MAXSCALEDENOM 30000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 8
      COLOR "#655949"
      OUTLINECOLOR "#F3EFEF"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
#      OFFSET 38 0
      PRIORITY 4
      WRAP " "
    END
  END
  CLASS
    EXPRESSION /^K2561/ # Golf
    STYLE
      COLOR "#C9DFAF"
    END
    LABEL
      STYLE
        SYMBOL "golf"
        SIZE 24
      END
      MAXSCALEDENOM 200000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#24471C"
      OUTLINECOLOR "#E5F0D8"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 4
      WRAP " "
    END
  END
  CLASS
    EXPRESSION /^K2(18.|190)/ # Parks
    STYLE
      COLOR "#C9DFAF"
    END
    LABEL
      STYLE
        SYMBOL "park"
        SIZE 16
      END
      MAXSCALEDENOM 200000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#24471C"
      OUTLINECOLOR "#E5F0D8"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 4
      WRAP " "
    END
  END
  CLASS
    EXPRESSION "K2361" # Shopping
    STYLE
      COLOR "#DEDAD2"
    END
    LABEL
      STYLE
        SYMBOL "shopping"
        SIZE 16
      END
      MAXSCALEDENOM 200000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#665A49"
      OUTLINECOLOR "#DAD9D6"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 4
      WRAP " "
    END
  END
  CLASS
    EXPRESSION "K2459" # Runway
    STYLE
      COLOR "#D3DDBD"
    END
  END
  CLASS
    EXPRESSION /^K254[03]/ # University, School
    STYLE
      COLOR "#E8DDBD"
    END
    LABEL
      STYLE
        SYMBOL "college"
        SIZE 16
      END
      MAXSCALEDENOM 200000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#665A49"
      OUTLINECOLOR "#DAD9D6"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 4
      WRAP " "
    END
  END
  CLASS
    EXPRESSION "K1121" # Buildings
    STYLE
      COLOR "#ECEBE8"
      OUTLINECOLOR "#DAD9D6"
      OUTLINEWIDTH 0.5
    END
    LABEL
      STYLE
        SYMBOL "building"
        SIZE 16
      END
      MAXSCALEDENOM 5000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#665A49"
      OUTLINECOLOR "#DFDBD4"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 4
      WRAP " "
    END
  END
  CLASS
    EXPRESSION "K2582" # Cemetery
    STYLE
      COLOR "#DFDBD4"
    END
    LABEL
#      STYLE
#        SYMBOL "etc/icons2/poi/9591.png"
#      END
      MAXSCALEDENOM 5000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#665A49"
      OUTLINECOLOR "#DFDBD4"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 4
      WRAP " "
    END
  END
  CLASS
    EXPRESSION "K2362" # Industrial
    STYLE
      COLOR "#E7E0D9"
    END
    LABEL
      STYLE
        SYMBOL "industrial"
        SIZE 16
      END
      MAXSCALEDENOM 30000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#665A49"
      OUTLINECOLOR "#DFDBD4"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 3
      WRAP " "
    END
  END
  CLASS
    EXPRESSION "K2110" # Military
    STYLE
      COLOR "#E7E0D9"
    END
    LABEL
      STYLE
        SYMBOL "military"
        SIZE 16
      END
      MAXSCALEDENOM 30000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#665A49"
      OUTLINECOLOR "#DFDBD4"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 3
      WRAP " "
    END
  END
  CLASS
    STYLE
      COLOR "#DFDBD4" # everything else
    END
    LABEL
      MAXSCALEDENOM 30000
      TYPE TRUETYPE
      FONT "sans"
      SIZE 7
      COLOR "#665A49"
      OUTLINECOLOR "#DFDBD4"
      PARTIALS FALSE
      MINDISTANCE 250
      MINFEATURESIZE 10
      BUFFER 4
      POSITION AUTO
      ALIGN AUTO
      PRIORITY 3
      WRAP " "
    END
  END
