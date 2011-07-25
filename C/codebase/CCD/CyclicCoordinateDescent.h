/*
 * CyclicCoordinateDescent.h
 *
 *  Created on: May-June, 2010
 *      Author: msuchard
 */

#ifndef CYCLICCOORDINATEDESCENT_H_
#define CYCLICCOORDINATEDESCENT_H_

#include "CompressedIndicatorMatrix.h"
#include "InputReader.h"

using namespace std;

#define DEBUG

#define TEST_SPARSE // New sparse updates are great
//#define TEST_ROW_INDEX
#define BETTER_LOOPS
#define MERGE_TRANSFORMATION
#define NEW_NUMERATOR

#ifdef DOUBLE_PRECISION
	typedef double real;
#else
	typedef float real;
#endif 

enum PriorType {
	LAPLACE = 0,
	NORMAL  = 1
};

enum ConvergenceType {
	LANGE = 0,
	ZHANG_OLES = 1
};

class CyclicCoordinateDescent {
	
public:
	
	CyclicCoordinateDescent(void);
	
	CyclicCoordinateDescent(			
			const char* fileNameX,
			const char* fileNameEta,
			const char* fileNameOffs,
			const char* fileNameNEvents,
			const char* fileNamePid			
		);
	
	CyclicCoordinateDescent(
			InputReader* reader
		);

	CyclicCoordinateDescent(
			int inN,
			CompressedIndicatorMatrix* inX,
			int* inEta, 
			int* inOffs, 
			int* inNEvents,
			int* inPid
		);
	
	void logResults(const char* fileName);

	virtual ~CyclicCoordinateDescent();
	
	double getLogLikelihood(void);

	double getPredictiveLogLikelihood(real* weights);

	double getLogPrior(void);
	
	virtual double getObjectiveFunction(void);

	real getBeta(int i);
		
	void update(int maxIterations, int convergenceType, double epsilon);

	virtual void resetBeta(void);

	// Setters
	void setHyperprior(double value);

	void setPriorType(int priorType);

	void setWeights(real* weights);

	// Getters
	string getPriorInfo();

	//tshaddox
	double classHierarchyVariance;  //variance at class level of hierarchy
	double sigma2Beta; //hyperprior variance (variance at drug coefficient level of hierarchy), tshaddox moved to unprotected
	bool useHierarchy;

	std::map<int, vector<int> > getChildMapCCD;
	std::map<int, int> getParentMapCCD;
	map<int, int> drugIdToIndexCCD;

protected:
	
//private:
	


	void init(void);
	
	void resetBounds(void);

	void computeXBeta(void);

	void saveXBeta(void);

	void computeXjEta(void);

	void computeSufficientStatistics(void);

	void updateSufficientStatistics(double delta, int index);

	void computeNumeratorForGradient(int index);

	virtual void computeNEvents(void);

	virtual void updateXBeta(double delta, int index);

	virtual void computeRemainingStatistics(bool);
	
	virtual void computeRatiosForGradientAndHessian(int index);

	virtual void computeRatio(int index);

	virtual void computeGradientAndHession(
			int index,
			double *gradient,
			double *hessian);

	virtual void getDenominators(void);

	double computeLogLikelihood(void);

	double getGradient(int drug);

	double getHessian(int drug);

	double getGradient_lasso(int drug);

	double getHessian_lasso(int drug);

	double ccdUpdateBeta(int index);
	
	double applyBounds(
			double inDelta,
			int index);
	
	double computeConvergenceCriterion(double newObjFxn, double oldObjFxn);
	
	virtual double computeZhangOlesConvergenceCriterion(void);

	template <class T>
	void zeroVector(T* vector, const int length) {
		for (int i = 0; i < length; i++) {
			vector[i] = 0;
		}
	}

	int getAlignedLength(int N);
		
	void testDimension(int givenValue, int trueValue, const char *parameterName);
	
	template <class T>
	void printVector(T* vector, const int length, ostream &os);
	
	double oneNorm(real* vector, const int length);
	
	double twoNormSquared(real * vector, const int length); 
	
	int sign(double x); 
	
	template <class T> 
	T* readVector(const char *fileName, int *length); 
			
	// Local variables
	
	//InputReader* hReader;
	
	ofstream outLog;
	bool hasLog;

	CompressedIndicatorMatrix* hXI; // K-by-J-indicator matrix	

	int* hOffs;  // K-vector
	int* hEta; // K-vector
	int* hNEvents; // K-vector
	int* hPid; // N-vector
	int** hXColumnRowIndicators; // J-vector
 	
	real* hBeta;
	real* hXBeta;
	real* hXBetaSave;
	real* hDelta;

	int N; // Number of patients
	int K; // Number of exposure levels
	int J; // Number of drugs
	

	string conditionId;

	int priorType;
	//double sigma2Beta; //Drug Coefficient variance
	double lambda;

	bool sufficientStatisticsKnown;

	bool validWeights;
	bool useCrossValidation;
	real* hWeights;

	// temporary variables
	real* expXBeta;
	real* offsExpXBeta;
	real* denomPid;
	real* numerPid;
	real* t1;
	real* xOffsExpXBeta;
	real* hXjEta;
};

double convertVarianceToHyperparameter(double variance);

#endif /* CYCLICCOORDINATEDESCENT_H_ */
