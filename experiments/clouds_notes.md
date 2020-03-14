# Clouds notes

Will try to have a few different systems: Heightmap / density map / SDF 2D cloud cover: displace cloud volume vertically like terrain, use SDF from voronoi cell noise to approximate optical depth (approximate AO too?). Can be used to cover entire sky. Voronoi can be created on GPU by drawing cones into depth buffer, making it easy to control cloud placement. Can displace slightly on cloud surface?

Small billboarded puffs to be used at a distance and faded out when close to camera

Raymarched or marching-simplices (marching cubes?) volume clouds: could use shell/fin fur techniques combined with optical depth approximation?

Can try to interact cloud layer with terrain slightly: example being tall peaks "splitting" cloud layer if there's strong wind