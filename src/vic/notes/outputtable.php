<?php

$cycle = 1;
$phi = 1;
$xcoord = 412;
for($i = 0; $i < 65; $i++) {

  print $cycle . "-1      ( " . $xcoord . " )\n";
  
  $xcoord = ($xcoord + 4) % 512;
  print $cycle . "-2      ( " . $xcoord . " )\n";
  $cycle++;
  $xcoord = ($xcoord + 4) % 512;
}