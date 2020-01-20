  # map-AREALM-L.inc
  MAXSCALEDENOM 5000000
  MINSCALEDENOM 2500000
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
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
#        SYMBOL "etc/icons2/poi/8060.png"
#      END
      MAXSCALEDENOM 10000
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
      PRIORITY 4
      WRAP " "
      ALIGN CENTER
    END
  END
  CLASS
    EXPRESSION /^K2561/ # Golf
    STYLE
      COLOR "#C9DFAF"
    END
    LABEL
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
#        SYMBOL "etc/icons2/poi/7921.png"
#      END
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
      PRIORITY 4
      WRAP " "
      ALIGN CENTER
    END
  END
  CLASS
    EXPRESSION /^K2(18.|190)/ # Parks
    STYLE
      COLOR "#C9DFAF"
    END
    LABEL
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
#        SYMBOL "etc/icons2/poi/7522.png"
#      END
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
      PRIORITY 4
      WRAP " "
      ALIGN CENTER
    END
  END
  CLASS
    EXPRESSION "K2361" # Shopping
    STYLE
      COLOR "#DEDAD2"
    END
    LABEL
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
#        SYMBOL "etc/icons2/poi/9987.png"
#      END
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
      PRIORITY 4
      WRAP " "
      ALIGN CENTER
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
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
#        SYMBOL "etc/icons2/poi/8200.png"
#      END
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
      PRIORITY 4
      WRAP " "
      ALIGN CENTER
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
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
#        SYMBOL "etc/icons2/poi/xxxx.png"
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
      PRIORITY 4
      WRAP " "
      ALIGN CENTER
    END
  END
  CLASS
    EXPRESSION "K2582" # Cemetery
    STYLE
      COLOR "#DFDBD4"
    END
    LABEL
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
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
      PRIORITY 4
      WRAP " "
      ALIGN CENTER
    END
  END
  CLASS
    EXPRESSION "K2362" # Industrial
    STYLE
      COLOR "#E7E0D9"
    END
    LABEL
#      STYLE
#        GEOMTRANSFORM 'labelpnt'
#        SYMBOL "etc/icons2/poi/9991.png"
#      END
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
      PRIORITY 3
      WRAP " "
      ALIGN CENTER
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
      PRIORITY 3
      WRAP " "
      ALIGN CENTER
    END
  END
