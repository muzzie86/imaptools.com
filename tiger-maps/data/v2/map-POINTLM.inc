#map-POINTLM.inc
    MAXSCALEDENOM 15000
    LABELITEM "FULLNAME"
    CLASSITEM "MTFCC"
    FILTER ('[MTFCC]' ne 'C3061')
#    CLASS # 264080
#      EXPRESSION /C3061/   # Cul-de-sac
#      LABEL
#        STYLE
#          SYMBOL "etc/icons2/poi/-3.png"
#        END
#        TYPE TRUETYPE
#        FONT "sans"
#        SIZE 8
#        PRIORITY 4
#        POSITION AUTO
#        WRAP " "
#        ALIGN CENTER
#        BUFFER 4
#        COLOR "#291509"
#        OUTLINECOLOR "#ffffff"
#        PARTIALS FALSE
#      END
#    END
    CLASS # 212815
      EXPRESSION /C3081/    # crossroad, community, populated place or locale
      LABEL
        STYLE
          SYMBOL "city_5"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 104401
      EXPRESSION /K2582/    # Cemetery
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9591.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 67967   # Misc. random stuff
      EXPRESSION /C3022|C3023|C3026|C3027|C3062|C3066|C3067|C3070|C3071|C3072|C3073|C3075|C3076|C3078|C3080|C3085|C3088|K2460/
      LABEL
        STYLE
          SYMBOL "circle-filled"
          COLOR "#818181"
          SIZE 5
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 12462   # Airports
      EXPRESSION /K2451|K2455|K2456/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/4581.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 9253    # University or College, School or Academy
      EXPRESSION /K2540|K2543/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/8200.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 2763    # Community Center, Government Center, Convention Center, City/Town Hall
      EXPRESSION /K2146|K2165|K2167|K2196/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9590.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 2678    # Parks, national, state, regionsl, county, private, other
      EXPRESSION /K2181|K2182|K2184|K2185|K2186|K2187|K2188|K2189|K2190/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/7522.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 2642    # Place of Worship; Convent, Monastery, Rectory, Other Religious Group Quarters
      EXPRESSION /K3544|K1239/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9992.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 2303    # Hospital/Hospice/ Urgent Care Facility
      EXPRESSION /K1231/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/8060.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 1185    # Golf Course
      EXPRESSION /K2561/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/7992.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 799     # Fire Department, Police Station
      EXPRESSION /K2193|K2194/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9993.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 462     # Local Jail or Detention Center; Federal Penitentiary, State Prison, or Prison Farm
      EXPRESSION /K1236|K1237/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/8231.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 379     # Crew-of-Vessel Location; Marine Terminal
      EXPRESSION /K1225|K2454/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/4482.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 353     # Library
      EXPRESSION /K2195/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9995.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 197     # Post Office
      EXPRESSION /K2191/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9530.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 169     # Train Station, Trolley or Mass Transit Rail Station
      EXPRESSION /K2452/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/4100.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 108     # Military Installation
      EXPRESSION /K2110/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/8410.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 38      # Bus Terminal
      EXPRESSION /K2453/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/4170.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 22      # Transportation Terminal
      EXPRESSION /K2400/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9593.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 17      # Museum, Visitor Center, Cultural Center, or Tourist Attraction; Zoo
      EXPRESSION /K2545|K2586/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9121.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 1       # Lighthouse Beacon
      EXPRESSION /C3074/
      LABEL
        STYLE
          SYMBOL "etc/icons2/poi/9715.png"
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN CENTER
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
