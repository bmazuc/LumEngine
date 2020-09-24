#include "MeshSceneNode.h"

MeshSceneNode::MeshSceneNode(Mesh* mesh)
	:mesh(mesh)
{
}


MeshSceneNode::~MeshSceneNode()
{

}

Mesh* MeshSceneNode::GetMesh()
{
	return mesh;
}
