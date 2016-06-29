

/*----------------------------------------------------------*/
/*															*/
/*				LIB OCTREE LOCALISATION V1.1				*/
/*															*/
/*----------------------------------------------------------*/
/*															*/
/*	Description:		Octree for mesh localization		*/
/*	Author:				Loic MARECHAL						*/
/*	Creation date:		mar 16 2012							*/
/*	Last modification:	apr 27 2015							*/
/*															*/
/*----------------------------------------------------------*/


/*----------------------------------------------------------*/
/* Includes													*/
/*----------------------------------------------------------*/

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <libmesh6.h>
#include <libol1.h>

#define BufSiz 100
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


/*----------------------------------------------------------*/
/* Load a mesh, build an octree & perform some localisations*/
/*----------------------------------------------------------*/

int main()
{
	LolInt i, j, k, cpt=0, NmbVer, NmbEdg, NmbTri, NmbItm, MshIdx, ver, dim, ref, idx;
    LolInt (*EdgTab)[2], (*TriTab)[3], buf[ BufSiz ], inc=50;
	long long OctIdx;
	double crd1[3] = {1.16235, 0.147997, -4.38923 }, crd2[3] = {0.002928, 0.079575, 0.006978}, crd3[3] = {-.0054, .1488, -.0067};
	double dis, (*VerTab)[3], MinCrd[3], MaxCrd[3], IncCrd[3], AvgDis=0;
	time_t t, t2, MinTim = INT_MAX, MaxTim = 0;


	/*---------------------------------------*/
	/* Open, allocate and read the mesh file */
	/*---------------------------------------*/

	t = clock();
	puts("\nRead mesh");

	if(!(MshIdx = GmfOpenMesh("winch.meshb", GmfRead, &ver, &dim)))
	{
		puts("Cannot open test.meshb.");
		return(1);
	}

	NmbVer = GmfStatKwd(MshIdx, GmfVertices);
	NmbEdg = GmfStatKwd(MshIdx, GmfEdges);
	NmbTri = GmfStatKwd(MshIdx, GmfTriangles);
	printf("vertices = %d, edges = %d, triangles = %d, dimension = %d, version = %d\n", NmbVer, NmbEdg, NmbTri, dim, ver);

	if( !NmbVer || (dim != 3) )
	{
		puts("Incompatible mesh.");
		return(1);
	}

	VerTab = malloc((NmbVer+1) * 3 * sizeof(double));
	GmfGotoKwd(MshIdx, GmfVertices);
	GmfGetBlock(MshIdx, GmfVertices, GmfDouble, &VerTab[1][0], &VerTab[2][0], GmfDouble, &VerTab[1][1], &VerTab[2][1], \
				GmfDouble, &VerTab[1][2], &VerTab[2][2], GmfInt, &ref, &ref);

	if(NmbEdg)
	{
		EdgTab = malloc((NmbEdg+1) * 2 * sizeof(LolInt));
		GmfGotoKwd(MshIdx, GmfEdges);
		GmfGetBlock(MshIdx, GmfEdges, GmfInt, &EdgTab[1][0], &EdgTab[2][0], \
                    GmfInt, &EdgTab[1][1], &EdgTab[2][1], GmfInt, &ref, &ref);
	}

	if(NmbTri)
	{
		TriTab = malloc((NmbTri+1) * 3 * sizeof(LolInt));
		GmfGotoKwd(MshIdx, GmfTriangles);
		GmfGetBlock(MshIdx, GmfTriangles, GmfInt, &TriTab[1][0], &TriTab[2][0], GmfInt, &TriTab[1][1], &TriTab[2][1], \
					GmfInt, &TriTab[1][2], &TriTab[2][2], GmfInt, &ref, &ref);
	}

	GmfCloseMesh(MshIdx);
	printf(" %g s\n", (double)(clock() - t) / CLOCKS_PER_SEC);


	/*---------------------------------------------------------*/
	/* Build an octree from this mesh and perform some queries */
	/*---------------------------------------------------------*/

	puts("\nBuild the octree : ");
	t = clock();
	OctIdx = LolNewOctree(  NmbVer, VerTab[1], VerTab[2], NmbEdg, EdgTab[1], EdgTab[2], NmbTri, TriTab[1], TriTab[2], \
                            0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL );

	printf(" %g s\n", (double)(clock() - t) / CLOCKS_PER_SEC);

	for(i=0;i<3;i++)
	{
		MinCrd[i] = crd2[i] - .0001;
		MaxCrd[i] = crd2[i] + .0001;
	}

	puts("\nSearch for vertices in a bounding box :");
	NmbItm = LolGetBoundingBox(OctIdx, LolTypVer, BufSiz, buf, MinCrd, MaxCrd);

	for(i=0;i<NmbItm;i++)
		printf(" vertex : %d\n", buf[i]);

    if(NmbEdg)
    {
        puts("\nSearch for the closest edge from a given point :");
	    idx = LolGetNearest(OctIdx, LolTypEdg, crd1, &dis, 0.);
	    printf(" closest edge = %d (%d - %d), distance = %g\n", idx, EdgTab[ idx ][0], EdgTab[ idx ][1], dis);
    }

	if(NmbTri)
	{
		puts("\nSearch for triangles in a bounding box :");

		NmbItm = LolGetBoundingBox(OctIdx, LolTypTri, BufSiz, buf, MinCrd, MaxCrd);

		for(i=0;i<NmbItm;i++)
			printf(" triangle : %d\n", buf[i]);

		puts("\nSearch for the closest triangle from a given point :");
		idx = LolGetNearest(OctIdx, LolTypTri, crd1, &dis, 0.);
		printf(" closest triangle = %d, distance = %g\n", idx, dis);

		for(i=1;i<=NmbVer;i++)
			for(j=0;j<3;j++)
				if(VerTab[i][j] < MinCrd[j])
					MinCrd[j] = VerTab[i][j];
				else if(VerTab[i][j] > MaxCrd[j])
					MaxCrd[j] = VerTab[i][j];

		for(i=0;i<3;i++)
			IncCrd[i] = (MaxCrd[i] - MinCrd[i]) / inc;

		crd1[0] = MinCrd[0];

		t = clock();

		for(i=0;i<inc;i++)
		{
			crd1[1] = MinCrd[1];

			for(j=0;j<inc;j++)
			{
				crd1[2] = MinCrd[2];

				for(k=0;k<inc;k++)
				{
					t2 = clock();
					idx = LolGetNearest(OctIdx, LolTypTri, crd1, &dis, .005);
					t2 = clock() - t2;
					MinTim = MIN(MinTim, t2);
					MaxTim = MAX(MaxTim, t2);
					AvgDis += dis;
					crd1[2] += IncCrd[2];
				}

				crd1[1] += IncCrd[1];
			}

			crd1[0] += IncCrd[0];
		}

		printf("nb samples = %d, mean dist = %g, total time = %g s, min time = %g s, max time = %g s\n", \
				inc*inc*inc, AvgDis / (inc*inc*inc), (double)(clock() - t) / CLOCKS_PER_SEC, \
				(double)MinTim / CLOCKS_PER_SEC, (double)MaxTim / CLOCKS_PER_SEC);
	}


	/*------------------*/
	/* Cleanup memories */ 
	/*------------------*/

	printf("\nFree octree %lld\n", OctIdx);
	printf(" memory used = %zd bytes\n", LolFreeOctree(OctIdx));
	free(VerTab);

	if(NmbTri)
		free(TriTab);

	if(NmbEdg)
		free(EdgTab);

	return(0);
}
