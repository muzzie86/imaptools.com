<?php
///// take a set of tiles, and a width and height, and composite together an image of all those tiles
///// the only output is the URL of the resulting document, be it image or PDF or whatnot

// where to place tempfiles?
define('TEMP_DIR', '/u/www/imaptools/images.tmp');
define('TEMP_URL', '/images.tmp/');


function imagecopymerge_alpha($dst_im, $src_im, $dst_x, $dst_y, $src_x, $src_y, $src_w, $src_h, $opacity){
    $w = imagesx($src_im);
    $h = imagesy($src_im);
    $cut = imagecreatetruecolor($src_w, $src_h);
    imagecopy($cut, $dst_im, 0, 0, $dst_x, $dst_y, $src_w, $src_h);
    imagecopy($cut, $src_im, 0, 0, $src_x, $src_y, $src_w, $src_h);
    imagecopymerge($dst_im, $cut, $dst_x, $dst_y, $src_x, $src_y, $src_w, $src_h, $opacity);
}

// fetch the request params
$width    = @$_REQUEST['width'];  if (!$width) $width = 1024;
$height   = @$_REQUEST['height']; if (!$height) $height = 768;
$tiles    = json_decode(@$_REQUEST['tiles']);
$random   = md5(microtime().mt_rand());
$file     = sprintf("%s/%s.jpg", TEMP_DIR, $random );
$url      = sprintf("%s/%s.jpg", TEMP_URL, $random );

// lay down the base image, then loop through the tiles
$image = imagecreatetruecolor($width,$height);
imagefill($image,0,0, imagecolorallocate($image,153,204,255) ); // fill with ocean blue

foreach ($tiles as $tile) {
   if (substr($tile->url,0,4)!=='http') {
      $tile->url = preg_replace('/^\.\//',dirname($_SERVER['REQUEST_URI']).'/',$tile->url);
      $tile->url = preg_replace('/^\.\.\//',dirname($_SERVER['REQUEST_URI']).'/../',$tile->url);
      $tile->url = sprintf("%s://%s:%d/%s", isset($_SERVER['HTTPS'])?'https':'http', $_SERVER['SERVER_ADDR'], $_SERVER['SERVER_PORT'], $tile->url);
   }
   $tile->url = str_replace(' ','+',$tile->url);

   // fetch the tile into a temp file, and analyze its type; bail if it's invalid
   $tempfile =  sprintf("%s/%s.img", TEMP_DIR, md5(microtime().mt_rand()) );
   file_put_contents($tempfile,file_get_contents($tile->url));
   list($tilewidth,$tileheight,$tileformat) = @getimagesize($tempfile);
   if (!$tileformat) continue;

   // load the tempfile's image, and blit it onto the canvas
   switch ($tileformat) {
      case IMAGETYPE_GIF:
         $tileimage = imagecreatefromgif($tempfile);
         break;
      case IMAGETYPE_JPEG:
         $tileimage = imagecreatefromjpeg($tempfile);
         break;
      case IMAGETYPE_PNG:
         $tileimage = imagecreatefrompng($tempfile);
         break;
   }

   imagecopymerge_alpha($image, $tileimage, $tile->x, $tile->y, 0, 0, $tilewidth, $tileheight, $tile->opacity );
}

// save to disk and tell the client where they can pick it up
imagejpeg($image,$file,100);
print $url;
?>
