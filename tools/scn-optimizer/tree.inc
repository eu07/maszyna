//---------------drzewo---------------
//parametry: tekstura, x, y, z, kat, wysokosc, rozpietosc 
origin (p2) (p3) (p4)
	rotate 0 (p5) 0

	node 500 0 none triangles (p1) 
	0,0,(p7)		0,0,0	1,0	end
	0,(p6),(p7)		0,0,0	1,1	end
	0,0,-(p7)		0,0,0	0,0	end
	0,(p6),-(p7)		0,0,0	0,1	end
	0,0,(p7)		0,0,0	1,0	end
	0,(p6),(p7)		0,0,0	1,1
	endtri

	node 600 0 none triangle_strip (p1) 
	-(p7),0,0		0,0,0	1,0	end
	-(p7),(p6),0		0,0,0	1,1	end
	(p7),0,0		0,0,0	0,0	end
	(p7),(p6),0		0,0,0	0,1	end
	-(p7),0,0		0,0,0	1,0	end
	-(p7),(p6),0		0,0,0	1,1
	endtri

	rotate 0 0 0
endorigin
