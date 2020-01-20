function printMap() {
  if (!confirm('Generating printable version.\nThis could take 1-2 minutes.\nPlease be patient.\nYou can continue to work while it is being generated.')) return;

  var printurl = 'print.php';
  // go through all layers, and collect a list of objects
  // each object is a tile's URL and the tile's pixel location relative to the viewport
  var tiles = [];
  for (var i=0; i<map.layers.length; i++) {
    // if the layer isn't visible at this range, or is turned off, skip it
    var layer = map.layers[i];
    if (!layer.grid) continue; // not a WMS layer anyway
    if (!layer.getVisibility()) continue;
    if (!layer.calculateInRange()) continue;

    // iterate through their grid's tiles, collecting each tile's extent and pixel location at this moment
    for (var p=0; p<layer.grid.length; p++) {
      for (var q=0; q<layer.grid[p].length; q++) {
        var tile     = layer.grid[p][q];
        var url      = layer.getURL(tile.bounds);
        var position = tile.position;
        var opacity  = layer.opacity ? parseInt(100*layer.opacity) : 100;
        var size     = tile.size;
        tiles[tiles.length] = {url:url, x:position.x, y:position.y, opacity:opacity, w:size.w, h:size.h};
      }
    }
  }

  var wkt = new OpenLayers.Format.WKT();

  var vectors = [];
  var vlayer = map.getLayersByName("Drawing Layer");
  if (vlayer.length>0) {
      var features = vlayer[0].features;
      for (var i=0; i<features.length; i++) {
        var f = features[i];
        if (f.onScreen() && f.getVisibility())
          vectors.push(wkt.write(features[i]));
      }
  }
  var vectors_json = JSON.stringify(vectors);

  var bounds = map.getExtent().toString();

  // hand off the list to our server-side script, which will do the heavy lifting
  var tiles_json = JSON.stringify(tiles);
  var size = map.getSize();
  var printparams = 'width='+size.w + '&height='+size.h + '&tiles='+escape(tiles_json) + '&bounds='+bounds + '&vectors='+escape(vectors_json) ;
//alert(tiles_json);
  //new OpenLayers.Ajax.Request(printurl, {method:'post', parameters:printparams, onComplete:printDone });
  OpenLayers.Request.issue({
    method: 'POST',
    url: printurl,
    headers: {
        "Content-Type": "application/x-www-form-urlencoded"
    },
    data: printparams,
    success: printDone
  });
}

function printDone(request) {
   var url = request.responseText;
   window.open(url);
}

