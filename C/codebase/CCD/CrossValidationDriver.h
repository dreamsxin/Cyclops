/*
 * CrossValidationDriver.h
 *
 *  Created on: Sep 10, 2010
 *      Author: msuchard
 */

#ifndef CROSSVALIDATIONDRIVER_H_
#define CROSSVALIDATIONDRIVER_H_

#include "CyclicCoordinateDescent.h"
#include "CrossValidationSelector.h"
#include "AbstractSelector.h"
#include "ccd.h"

#include "AbstractDriver.h"

namespace bsccs {
class CrossValidationDriver : public AbstractDriver {
public:
	CrossValidationDriver(
			int iGridSize,
			double iLowerLimit,
			double iUpperLimit);

	virtual ~CrossValidationDriver();

	virtual void drive(
			CyclicCoordinateDescent& ccd,
			AbstractSelector& selector,
			const CCDArguments& arguments);

	void changeParameter(CyclicCoordinateDescent &ccd, int varianceIndex, double varianceValue);

	double oneFoldCrossValidation(CyclicCoordinateDescent& ccd,
			AbstractSelector& selector,
			const CCDArguments& arguments, int i, vector<bsccs::real> weights, int step, int point);

	void greedyDrive(CyclicCoordinateDescent& ccd,
			AbstractSelector& selector,
			const CCDArguments& arguments);

	void hierarchyDrive(CyclicCoordinateDescent& ccd,
			AbstractSelector& selector,
			const CCDArguments& arguments);

	void resetForOptimal(
			CyclicCoordinateDescent& ccd,
			CrossValidationSelector& selector,
			const CCDArguments& arguments);

	virtual void logResults(const CCDArguments& arguments);

private:

	double computeGridPoint(int step);

	double computePointEstimate(const std::vector<double>& value);

	void findMax(double* maxPoint, double* maxValue, double* minValue);

	std::vector<double> gridPoint;
	std::vector<double> gridValue;

	int gridSize;
	double lowerLimit;
	double upperLimit;

};
}
#endif /* CROSSVALIDATIONDRIVER_H_ */
