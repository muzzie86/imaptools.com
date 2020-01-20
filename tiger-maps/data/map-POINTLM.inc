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
#        ALIGN AUTO
#        WRAP " "
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
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 212815
      EXPRESSION /T3081/    # Shopping
      LABEL
        STYLE
          SYMBOL "shopping"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "cemetery"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 63490   # Peak
      EXPRESSION /^C3022/
      LABEL
        STYLE
          SYMBOL "triangle"
          COLOR "#818181"
          SIZE 8
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 4477   # Campground
      EXPRESSION /T3082/
      LABEL
        STYLE
          SYMBOL "camping"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 4477   # Misc. random stuff
      EXPRESSION /C3023|C3026|C3027|C3062|C3067|C3075|C3076|C3078|C3080|C3085|C3088|K2460/
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
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "airport"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 9253    # University or College, School or Academy
      EXPRESSION /^K254[03]/
      LABEL
        STYLE
          SYMBOL "college"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "government"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "park"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "church"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "hospital"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "golf"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "fire-station"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "prison"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "port"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "library"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "postoffice"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "rail-station"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
          SYMBOL "military"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        WRAP " "
        ALIGN AUTO
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
          SYMBOL "bus-station"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 16      # Museum, Visitor Center, Cultural Center, or Tourist Attraction
      EXPRESSION /K2545/
      LABEL
        STYLE
          SYMBOL "museum"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS # 1       # Zoo
      EXPRESSION /K2586/
      LABEL
        STYLE
          SYMBOL "zoo"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
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
        ALIGN AUTO
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS
      EXPRESSION /C3073/
      LABEL
        STYLE
          SYMBOL "tower-water"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS
      EXPRESSION /C3072/
      LABEL
        STYLE
          SYMBOL "tower"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS
      EXPRESSION /^C307[01]/
      LABEL
        STYLE
          SYMBOL "tower-lookout"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
    CLASS
      EXPRESSION /^C3066/
      LABEL
        STYLE
          SYMBOL "gate"
          SIZE 16
        END
        TYPE TRUETYPE
        FONT "sans"
        SIZE 8
        PRIORITY 4
        POSITION AUTO
        ALIGN AUTO
        WRAP " "
        BUFFER 4
        COLOR "#291509"
        OUTLINECOLOR "#ffffff"
        PARTIALS FALSE
      END
    END
