#include <Scripts/DefaultScript.h>
#include "stdafx.h"


#include "Globals.h"
#include "Logs.h"
#include "vertex.h"
#include "scenenode.h"
#include "renderer.h"
#include "openglrenderer.h"
#include "simulation.cpp"
#include "application.h"
#include "scenarioloadermode.h"

bool StartDone = false;
void Start() // Called at start of program
{
	if (StartDone == false)
	{
		WriteLog("Scripting start EVENT");

		world_vertex vert1, vert2, vert3;
		vert1.position.x = 0;
		vert1.position.y = 0;
		vert1.position.z = 10;

		vert2.position.x = 0;
		vert2.position.y = 10;
		vert2.position.z = 10;

		vert3.position.x = 10;
		vert3.position.y = 10;
		vert3.position.z = 10;

		scene::shape_node gameObject;
		scene::shape_node::shapenode_data mesh;
		scene::scratch_data _scratchpad;

		// definicja mesha:
		mesh.rangesquared_max = 100;
		mesh.rangesquared_min = -1;
		mesh.origin = glm::dvec3{0, 0, 0};
		mesh.vertices.emplace_back(vert1);
		mesh.vertices.emplace_back(vert2);
		mesh.vertices.emplace_back(vert3);

		gameObject.setMesh(mesh);

		_scratchpad.name = "none";


		simulation::Region->insert(gameObject, _scratchpad, true);
		StartDone = true;
	}
	
}
void Update() // Called Every simulation update
{

}