	/*
 * HierarchyReader.cpp
 *
 *  Created on: Jun 7, 2011
 *      Author: tshaddox
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "HierarchyReader.h"
#include "CyclicCoordinateDescent.h"

#define MAX_ENTRIES		1000000000

#define FORMAT_MATCH_1	"\"ingredientIds\""
#define MATCH_LENGTH_1	15


#define MISSING_STRING	"NA"

#ifdef USE_DRUG_STRING
	#define NO_DRUG			"0"
#else
	#define	NO_DRUG			0
#endif

using namespace std;


HierarchyReader::HierarchyReader() {

}


HierarchyReader::HierarchyReader(const char* fileName, map<int, int> &drugIdToIndex, CyclicCoordinateDescent **ccd) {
	//cout << "fileName = " << fileName << endl;
	ifstream in(fileName);
	if (!in) {
		cerr << "Unable to open " << fileName << endl;
		exit(-1);
	}

	string line;
	getline(in, line); // Read header

	if (line.compare(0, MATCH_LENGTH_1, FORMAT_MATCH_1) != 0) {
		cerr << "Unrecognized file type" << endl;
		exit(-1);
	}

	int currentEntry = 0;

	while (getline(in, line) && (currentEntry < MAX_ENTRIES)) {
		int parent;
		int child;
		string meaningless;
		if (!line.empty()) {
			stringstream ss(line.c_str());
			ss >> meaningless;
			ss >> child; //ingredientId

			if (drugIdToIndex[child] != NULL){
				child = drugIdToIndex[child] - 1;
				ss >> parent; //ATC group

				if (parent == 0) { //if not associated with a group, the group and ingredient have same id num
					(*ccd)->getChildMapCCD[child].push_back(child);
					(*ccd)->getParentMapCCD[child] = child;
				} else {
					(*ccd)->getChildMapCCD[parent].push_back(child);
					(*ccd)->getParentMapCCD[child] = parent;
				}
			}
		}
	}
}

