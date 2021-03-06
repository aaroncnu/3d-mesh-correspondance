#include <core/TriangularMesh.h>
#include <core/PathUtil.h>
#include <core/NDimVector.h>
#include <core/ColorPalette.h>
#include <core/TriMeshAuxInfo.h>

namespace TAShape
{
	TriangularMesh::TriangularMesh()
	{

	}

	TriangularMesh::~TriangularMesh()
	{
		clear();
	}

	TACore::Result TriangularMesh::clear()
	{
		for (size_t i = 0; i < verts.size(); i++)
		{
			TACORE_SAFE_DELETE(verts[i]);
		}
		for (size_t i = 0; i < tris.size(); i++)
		{
			TACORE_SAFE_DELETE(tris[i]);
		}
		for (size_t i = 0; i < edges.size(); i++)
		{
			TACORE_SAFE_DELETE(edges[i]);
		}
		verts.clear();
		tris.clear();
		edges.clear();
		return TACore::TACORE_OK;
	}

	TACore::Result TriangularMesh::load(const char* fName)
	{
		const std::string ext = TACore::PathUtil::getExtension(std::string(fName));
		if (ext == "obj")
		{
			return loadObj(fName);
		}
		else if (ext == "off")
		{
			return loadOff(fName);
		}
		return TACore::TACORE_INVALID_OPERATION;
	}

	TACore::Result TriangularMesh::loadObj(const char* meshFile)
	{
		FILE* fPtr;
		if (!(fPtr = fopen(meshFile, "r")))
		{
			return TACore::TACORE_FILE_ERROR;
		}

		//Clear old mesh
		clear();

		char type;	//type of line: vertex line (v) or face line (f)
		float a, b, c;	//for face lines casting these to int will suffice
		/*minEdgeLen = INF;
		maxEdgeLen = -INF;
		edgeLenTotal = 0.0f;*/
		while (fscanf(fPtr, "%c %f %f %f\n", &type, &a, &b, &c) != EOF) //go till the end of file
			if (type == 'v')
			{
			//assume none of verts are duplicated initially
			//a *= SCALE_FACTOR;
			//b *= SCALE_FACTOR;
			//c *= SCALE_FACTOR;
			float* coords = new float[3];
			coords[0] = a;
			coords[1] = b;
			coords[2] = c;
			addVertex(coords); //ND: no duplicate check
			}
			else //if (type == 'f')
			{
				addTriangle((int)a - 1, (int)b - 1, (int)c - 1); //-1: obj indices start at 1
				verts[(int)a - 1]->interior = verts[(int)b - 1]->interior = verts[(int)c - 1]->interior = false;
			}
	
		for (int v = 0; v < (int)verts.size(); v++)
		{
			if (verts[v]->triList.empty())
			{
				std::cout << "2D Dirichlet/ARAP regularization behaves weirdly w/ isolated vertices: get rid of v" << v << std::endl;
				fclose(fPtr);
				return TACore::TACORE_ERROR;
			}
		}
		fclose(fPtr);
		std::cout << "Mesh has " << (int)tris.size() << " tris, " << (int)verts.size() << " verts, " << (int)edges.size() << " edges\nInitialization done\n";
		return TACore::TACORE_OK;
	}

	TACore::Result TriangularMesh::loadOff(const char* fName)
	{
		FILE* fPtr;
		if (!(fPtr = fopen(fName, "r")))
		{
			return TACore::TACORE_FILE_ERROR;
		}

		//Clear old mesh
		clear();

		char off[25];
		fscanf(fPtr, "%s\n", &off); //cout << off << " type file\n";
		float a, b, c, d;	//for face lines and the 2nd line (that gives # of verts, faces, and edges) casting these to int will suffice
		fscanf(fPtr, "%f %f %f\n", &a, &b, &c);
		int nVerts = (int)a, v = 0;
		/*	minEdgeLen = INF;
		maxEdgeLen = -INF;
		edgeLenTotal = 0.0f;*/
		while (v++ < nVerts) //go till the end of verts coord section
		{
			fscanf(fPtr, "%f %f %f\n", &a, &b, &c);


			float* coords = new float[3];
			coords[0] = a;
			coords[1] = b;
			coords[2] = c;

			addVertex(coords);
		}
		//verts ready, time to fill triangles
		while (fscanf(fPtr, "%f %f %f %f\n", &d, &a, &b, &c) != EOF) //go till the end of file
		{
			addTriangle((int)a, (int)b, (int)c); //no -1 'cos idxs start from 0 for off files
		}

		for (int v = 0; v < (int)verts.size(); v++)
		{
			if (verts[v]->triList.empty())
			{
				std::cout << "2D Dirichlet/ARAP regularization behaves weirdly w/ isolated vertices: get rid of v" << v << std::endl;
				fclose(fPtr);
				return TACore::TACORE_ERROR;
			}
		}
		fclose(fPtr);

		std::cout << "Mesh has " << (int)tris.size() << " tris, " << (int)verts.size() << " verts, " << (int)edges.size() << " edges\nInitialization done\n";
		return TACore::TACORE_OK;
	}

	TACore::Result TriangularMesh::save(const char* fName, const std::vector<double>& pVertMagnitudes)
	{
		const std::string ext = TACore::PathUtil::getExtension(std::string(fName));
		if (ext == "ply")
		{
			return savePly(fName, pVertMagnitudes);
		}
		return TACore::TACORE_INVALID_OPERATION;
	}

	TACore::Result TriangularMesh::savePly(const char* fName, const std::vector<double>& pVertMagnitudes)
	{
		TACORE_CHECK_ARGS(pVertMagnitudes.size() == 0 || pVertMagnitudes.size() == verts.size());

		FILE* fPtr;
		if (!(fPtr = fopen(fName, "w")))
		{
			return TACore::TACORE_FILE_ERROR;
		}
		
		fprintf(fPtr, "ply\n");
		fprintf(fPtr, "format ascii 1.0\n");
		fprintf(fPtr, "element vertex %d\n", verts.size());
		fprintf(fPtr, "property float32 x\n");
		fprintf(fPtr, "property float32 y\n");
		fprintf(fPtr, "property float32 z\n");
		fprintf(fPtr, "property uchar diffuse_red\n");
		fprintf(fPtr, "property uchar diffuse_green\n");
		fprintf(fPtr, "property uchar diffuse_blue\n");
		fprintf(fPtr, "element face %d\n", tris.size());
		fprintf(fPtr, "property list uint8 int32 vertex_indices\n");
		fprintf(fPtr, "end_header\n");

		TACore::ColorPalette colorPalette(1.0f / verts.size());

		for (int v = 0; v < verts.size(); v++)
		{
			fprintf(fPtr, "%f %f %f ", verts[v]->coords[0], verts[v]->coords[1], verts[v]->coords[2]);
			if (pVertMagnitudes.size() > 0)
			{
				unsigned char r, g, b;
				colorPalette.getColor(pVertMagnitudes[v], r, g, b);
				fprintf(fPtr, "%d %d %d ", (int)r, (int)g, (int)b);
			}
			fprintf(fPtr, "\n");
		}

		for (int t = 0; t < tris.size(); t++)
		{
			fprintf(fPtr, "%d %d %d %d\n", 3, tris[t]->v1i, tris[t]->v2i, tris[t]->v3i);
		}

		fclose(fPtr);
		return TACore::TACORE_OK;
	}


	TACore::Result TriangularMesh::createCube(float sideLength)
	{
		float** coords = new float*[8], flbc[3] = { 10, 7, 5 }, delta[3] = { 0, 0, 0 };
		for (int v = 0; v < 8; v++)
		{
			coords[v] = new float[3];

			if (v == 1)
				delta[0] += sideLength;
			else if (v == 2)
				delta[1] += sideLength;
			else if (v == 3)
				delta[0] -= sideLength;
			else if (v == 4)
				delta[2] -= sideLength;
			else if (v == 5)
				delta[0] += sideLength;
			else if (v == 6)
				delta[1] -= sideLength;
			else if (v == 7)
				delta[0] -= sideLength;

			for (int c = 0; c < 3; c++)
				coords[v][c] = flbc[c] + delta[c];

			addVertex(coords[v]);
		}

		//connectivity
		addTriangle(0, 1, 2);
		addTriangle(0, 2, 3);

		addTriangle(1, 6, 2);
		addTriangle(6, 5, 2);

		addTriangle(7, 5, 6);
		addTriangle(7, 4, 5);

		addTriangle(0, 3, 7);
		addTriangle(3, 4, 7);

		addTriangle(0, 6, 1);
		addTriangle(0, 7, 6);

		addTriangle(3, 2, 5);
		addTriangle(3, 5, 4);

		return TACore::TACORE_OK;
	}

	TACore::Result TriangularMesh::addVertex(float* coords)
	{
		int idx = verts.size();
		verts.push_back(new Vertex(idx, coords));
		return TACore::TACORE_OK;
	}

	TACore::Result TriangularMesh::addTriangle(int v1i, int v2i, int v3i)
	{
		int idx = tris.size();
		tris.push_back(new Triangle(idx, v1i, v2i, v3i));

		verts[v1i]->triList.push_back(idx);
		verts[v2i]->triList.push_back(idx);
		verts[v3i]->triList.push_back(idx);

		tris[idx]->e1 = makeVertsNeighbors(v1i, v2i);
		tris[idx]->e2 = makeVertsNeighbors(v1i, v3i);
		tris[idx]->e3 = makeVertsNeighbors(v2i, v3i);

		return TACore::TACORE_OK;
	}

	int TriangularMesh::makeVertsNeighbors(int v, int w)
	{
		//try to make v and w neighbor; return the edge id if they already are
		for (int e = 0; e < (int) verts[v]->edgeList.size(); e++)
		{
			if (edges[verts[v]->edgeList[e]]->v1i == w || edges[verts[v]->edgeList[e]]->v2i == w)
			{
				return verts[v]->edgeList[e];
			}
		}

		verts[v]->vertList.push_back(w);
		verts[w]->vertList.push_back(v);
		return addEdge(v, w);
	}

	float TriangularMesh::eucDistanceBetween(Vertex *v1, Vertex *v2) const
	{
		float diffZero = v1->coords[0] - v2->coords[0];
		float diffOne = v1->coords[1] - v2->coords[1];
		float diffTwo = v1->coords[2] - v2->coords[2];
		return (float)sqrt(diffZero*diffZero + diffOne*diffOne + diffTwo*diffTwo);
	}

	int TriangularMesh::addEdge(int a, int b)
	{
		int idx = edges.size();
		edges.push_back(new Edge(idx, a, b, eucDistanceBetween(verts[a], verts[b])));

		verts[a]->edgeList.push_back(idx);
		verts[b]->edgeList.push_back(idx);

		return idx;
	}

	TACore::Result TriangularMesh::assignNormalsToTriangles()
	{
		for (int i = 0; i < (int) this->tris.size(); i++)
		{
			TAVector pva; pva.x = this->verts[this->tris[i]->v1i]->coords[0]; pva.y = this->verts[this->tris[i]->v1i]->coords[1]; pva.z = this->verts[this->tris[i]->v1i]->coords[2];
			TAVector pvb; pvb.x = this->verts[this->tris[i]->v2i]->coords[0]; pvb.y = this->verts[this->tris[i]->v2i]->coords[1]; pvb.z = this->verts[this->tris[i]->v2i]->coords[2];
			TAVector pvc; pvc.x = this->verts[this->tris[i]->v3i]->coords[0]; pvc.y = this->verts[this->tris[i]->v3i]->coords[1]; pvc.z = this->verts[this->tris[i]->v3i]->coords[2];

			TAVector normal;
			TAVector lhs;
			TAVector rhs;
			lhs.x = pvb.x - pva.x;
			lhs.y = pvb.y - pva.y;
			lhs.z = pvb.z - pva.z;

			rhs.x = pvc.x - pva.x;
			rhs.y = pvc.y - pva.y;
			rhs.z = pvc.z - pva.z;

			normal = lhs * rhs;
			normal.normalize();

			this->tris[i]->tNormal = normal;

		}
		return TACore::TACORE_OK;
	}

	float TriangularMesh::calcAreaOfTriangle(int t) const
	{
		Triangle *tp = tris[t];
		TACore::Vector3D A(verts[tp->v1i]->coords[0], verts[tp->v1i]->coords[1], verts[tp->v1i]->coords[2]);
		TACore::Vector3D B(verts[tp->v2i]->coords[0], verts[tp->v2i]->coords[1], verts[tp->v2i]->coords[2]);
		TACore::Vector3D C(verts[tp->v3i]->coords[0], verts[tp->v3i]->coords[1], verts[tp->v3i]->coords[2]);

		TACore::Vector3D AB(A, B);
		TACore::Vector3D AC(A, C);

		double angle = TACore::Vector3D::getAngleBetweenTwo(AB, AC);

		return float (.5 * AB.norm() * AC.norm() * sin(angle));
	}

	void TriangularMesh::calcRingAreasOfVertices(std::vector<double>& ringAreas) const
	{
		ringAreas = std::vector<double>(verts.size());
		std::vector<double> triangleAreas(tris.size());
		for (int t = 0; t < (int)tris.size(); t++)
		{
			triangleAreas[t] = calcAreaOfTriangle(t);
		}

		for (int v = 0; v < (int)verts.size(); v++)
		{
			double sumAreas = 0.0;
			for (int tv = 0; tv < (int)verts[v]->triList.size(); tv++)
			{
				sumAreas += triangleAreas[verts[v]->triList[tv]];
			}
			ringAreas[v] = sumAreas;
		}
	}

	float TriangularMesh::calcAverageEdgeLength() const
	{
		float sum = 0.0f;
		for (size_t e = 0; e < edges.size(); e++)
		{
			sum += edges[e]->length;
		}
		return sum / edges.size();
	}

	float TriangularMesh::calcTotalSurfaceArea() const
	{
		float sum = 0.0f;
		for (size_t t = 0; t < tris.size(); t++)
		{
			sum += calcAreaOfTriangle(t);
		}
		return sum;
	}

	float TriangularMesh::calcMaxEucDistanceBetweenTwoVertices() const
	{
		float maxEucDistance = 0.0f;
		for (int v = 0; v < verts.size(); v++)
		{
			if (v < verts.size() - 1)
			{
				std::cout << "%" << (100 * v) / verts.size() << " completed for calculating MaxEucDistanceBetweenTwoVertices" << "\r";
			}
			else
			{
				std::cout << "%" << 100 << " completed for calculating MaxEucDistanceBetweenTwoVertices" << "\n";
			}

			for (int w = 0; w < v; w++)
			{
				const float currEucDistance = eucDistanceBetween(verts[v], verts[w]);
				if (currEucDistance > maxEucDistance)
				{
					maxEucDistance = currEucDistance;
				}
			}
		}
		return maxEucDistance;
	}

	float TriangularMesh::calcMaxAreaBetweenThreeVertices() const
	{
		float maxArea = 0.0f;
		for (int v = 0; v < verts.size(); v++)
		{
			if (v < verts.size() - 1)
			{
				std::cout << "%" << (100 * v) / verts.size() << " completed for calculating MaxAreaBetweenThreeVertices" << "\r";
			}
			else
			{
				std::cout << "%" << 100 << " completed for calculating MaxAreaBetweenThreeVertices" << "\n";
			}

			for (int w = 0; w < v; w++)
			{
				for (int u = 0; u < w; u++)
				{
					TACore::Vector3D A(verts[v]->coords[0], verts[v]->coords[1], verts[v]->coords[2]);
					TACore::Vector3D B(verts[w]->coords[0], verts[w]->coords[1], verts[w]->coords[2]);
					TACore::Vector3D C(verts[u]->coords[0], verts[u]->coords[1], verts[u]->coords[2]);

					TACore::Vector3D AB(A, B);
					TACore::Vector3D AC(A, C);

					double angle = TACore::Vector3D::getAngleBetweenTwo(AB, AC);

					float currArea = float(.5 * AB.norm() * AC.norm() * sin(angle));

					if (currArea > maxArea)
					{
						maxArea = currArea;
					}
				}
			}
		}
		return maxArea;
	}

	float TriangularMesh::calcMaxVolumeOfTetrahedronBetweenFourVertices() const
	{
		float maxVolume = 0.0f;
		for (int v = 0; v < verts.size(); v++)
		{
			if (v < verts.size() - 1)
			{
				std::cout << "%" << (100 * v) / verts.size() << " completed for calculating MaxVolumeOfTetrahedronBetweenFourVertices" << "\r";
			}
			else
			{
				std::cout << "%" << 100 << " completed for calculating MaxVolumeOfTetrahedronBetweenFourVertices" << "\n";
			}

			for (int w = 0; w < v; w++)
			{
				for (int u = 0; u < w; u++)
				{
					for (int y = 0; y < u; y++)
					{
						TACore::Vector3D A(verts[v]->coords[0], verts[v]->coords[1], verts[v]->coords[2]);
						TACore::Vector3D B(verts[w]->coords[0], verts[w]->coords[1], verts[w]->coords[2]);
						TACore::Vector3D C(verts[u]->coords[0], verts[u]->coords[1], verts[u]->coords[2]);
						TACore::Vector3D D(verts[y]->coords[0], verts[y]->coords[1], verts[y]->coords[2]);

						TACore::Vector3D pt1ToPt4(A, D);
						TACore::Vector3D pt2ToPt4(B, D);
						TACore::Vector3D pt3ToPt4(C, D);

						float currVolume = abs(pt1ToPt4 % (pt2ToPt4 * pt3ToPt4)) / 6.0;

						if (currVolume > maxVolume)
						{
							maxVolume = currVolume;
						}
					}
				}
			}
		}
		return maxVolume;
	}

	TriMeshAuxInfo TriangularMesh::calcAuxInfo() const
	{
		TriMeshAuxInfo res;
		res.m_lfMaxEucDistanceBetweenTwoVertices = calcMaxEucDistanceBetweenTwoVertices();
		res.m_lfMaxAreaOfTriangleConstructedByThreeVertices = calcMaxAreaBetweenThreeVertices();
		res.m_lfMaxVolumeOfTetrahedronConstructedByForVertices = calcMaxVolumeOfTetrahedronBetweenFourVertices();
		return res;
	}
}