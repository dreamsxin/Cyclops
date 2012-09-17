/*
 * BootstrapDriver.cpp
 *
 *  Created on: Nov 17, 2010
 *      Author: msuchard
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <algorithm>
#include <cstdlib>
#include <cmath>

#include "BootstrapDriver.h"
#include "AbstractSelector.h"

BootstrapDriver::BootstrapDriver(
		int inReplicates,
		ModelData* inModelData) : replicates(inReplicates), modelData(inModelData),
		J(inModelData->getNumberOfColumns()) {

	// Set-up storage for bootstrap estimates
	estimates.resize(J);
	int count = 0;
	for (rarrayIterator it = estimates.begin(); it != estimates.end(); ++it) {
		*it = new rvector();
	}
}

BootstrapDriver::~BootstrapDriver() {
	for (rarrayIterator it = estimates.begin(); it != estimates.end(); ++it) {
		if (*it) {
			delete *it;
		}
	}
}

void BootstrapDriver::drive(
		CyclicCoordinateDescent& ccd,
		AbstractSelector& selector,
		const CCDArguments& arguments) {

	// TODO Make sure that selector is type-of BootstrapSelector
	std::vector<real> weights;

	for (int step = 0; step < replicates; step++) {
		selector.permute();
		selector.getWeights(0, weights);
		ccd.setWeights(&weights[0]);

		std::cout << std::endl << "Running replicate #" << (step + 1) << std::endl;
		// Run CCD using a warm start
		ccd.update(arguments.maxIterations, arguments.convergenceType, arguments.tolerance);

		// Store point estimates
		for (int j = 0; j < J; ++j) {
			estimates[j]->push_back(ccd.getBeta(j));
		}
		exit(-1); // ***
	}
}

void BootstrapDriver::logResults(const CCDArguments& arguments) {
	fprintf(stderr,"Not yet implemented.\n");
	exit(-1);
}

void BootstrapDriver::logResults(const CCDArguments& arguments, std::vector<real>& savedBeta, std::string conditionId) {

	ofstream outLog(arguments.outFileName.c_str());
	if (!outLog) {
		cerr << "Unable to open log file: " << arguments.bsFileName << endl;
		exit(-1);
	}

	string sep(","); // TODO Make option

	if (!arguments.reportRawEstimates) {
		outLog << "Drug_concept_id" << sep << "Condition_concept_id" << sep <<
				"score" << sep << "standard_error" << sep << "bs_mean" << sep << "bs_lower" << sep <<
				"bs_upper" << sep << "bs_prob0" << endl;
	}

	for (int j = 0; j < J; ++j) {
		outLog << modelData->getColumn(j).getLabel() <<
			sep << conditionId << sep;
		if (arguments.reportRawEstimates) {
			ostream_iterator<real> output(outLog, sep.c_str());
			copy(estimates[j]->begin(), estimates[j]->end(), output);
			outLog << endl;
		} else {
			real mean = 0.0;
			real var = 0.0;
			real prob0 = 0.0;
			for (rvector::iterator it = estimates[j]->begin(); it != estimates[j]->end(); ++it) {
				mean += *it;
				var += *it * *it;
				if (*it == 0.0) {
					prob0 += 1.0;
				}
			}

			real size = static_cast<real>(estimates[j]->size());
			mean /= size;
			var = (var / size) - (mean * mean);
			prob0 /= size;

			sort(estimates[j]->begin(), estimates[j]->end());
			int offsetLower = static_cast<int>(size * 0.025);
			int offsetUpper = static_cast<int>(size * 0.975);

			real lower = *(estimates[j]->begin() + offsetLower);
			real upper = *(estimates[j]->begin() + offsetUpper);

			outLog << savedBeta[j] << sep;
			outLog << std::sqrt(var) << sep << mean << sep << lower << sep << upper << sep << prob0 << endl;
		}
	}
	outLog.close();
}
