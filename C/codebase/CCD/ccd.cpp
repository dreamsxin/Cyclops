/*
 * ccd.cpp
 *
 *  Created on: July, 2010
 *      Author: msuchard
 */


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <time.h>
#include <sys/time.h>

#include <math.h>

#include "ccd.h"
#include "CyclicCoordinateDescent.h"
#include "InputReader.h"
#include "CrossValidationSelector.h"
#include "CrossValidationDriver.h"
#include "BootstrapSelector.h"
#include "BootstrapDriver.h"

#include "HierarchyReader.h"

#include "tclap/CmdLine.h"

#ifdef CUDA
	#include "GPUCyclicCoordinateDescent.h"
	#include "BetterGPU.h"
#endif


#define NEW

using namespace TCLAP;
using namespace std;

double calculateSeconds(const timeval &time1, const timeval &time2) {
	return time2.tv_sec - time1.tv_sec +
			(double)(time2.tv_usec - time1.tv_usec) / 1000000.0;
}

void parseCommandLine(int argc, char* argv[], CCDArguments &arguments) {
	try {
		CmdLine cmd("Cyclic coordinate descent algorithm for self-controlled case studies", ' ', "0.1");
		ValueArg<int> gpuArg("g","GPU","Use GPU device", false, -1, "device #");
		SwitchArg betterGPUArg("1","better", "Use better GPU implementation", false);
		ValueArg<int> maxIterationsArg("", "maxIterations", "Maximum iterations", false, 100, "int");
		UnlabeledValueArg<string> inFileArg("inFileName","Input file name", true, "default", "inFileName");
		UnlabeledValueArg<string> outFileArg("outFileName","Output file name", true, "default", "outFileName");

		// Prior arguments
		ValueArg<double> hyperPriorArg("v", "variance", "Hyperprior variance", false, 1.0, "real");
		SwitchArg normalPriorArg("n", "normalPrior", "Use normal prior, default is laplace", false);

		// Hierarchy arguments by tshaddox
		ValueArg<string> hierarchyFileArg("a", "hierarchyFile", "Hierarchy file name", false, "noFileName", "hierarchyFile");
		ValueArg<double> classHierarchyVarianceArg("d","classHierarchyVariance","Variance for drug class hierarchy", false, 1000, "Variance at the class level of the hierarchy");
		ValueArg<double> sigma2BetaArg("e","sigma2Beta","Variance for drug coefficients", false, 1000, "Variance at the drug level of the hierarchy (hyperprior variance)");

		// Convergence criterion arguments
		ValueArg<double> toleranceArg("t", "tolerance", "Convergence criterion tolerance", false, 1E-4, "real");
		SwitchArg zhangOlesConvergenceArg("z", "zhangOles", "Use Zhange-Oles convergence criterion, default is false", false);
		ValueArg<long> seedArg("s", "seed", "Random number generator seed", false, 0, "long");

		// Cross-validation arguments
		SwitchArg doCVArg("c", "cv", "Perform cross-validation selection of hyperprior variance", false);
		ValueArg<double> lowerCVArg("l", "lower", "Lower limit for cross-validation search", false, 1.0, "real");
		ValueArg<double> upperCVArg("u", "upper", "Upper limit for cross-validation search", false, 10.0, "real");
		ValueArg<int> foldCVArg("f", "fold", "Fold level for cross-validation", false, 10, "int");
		ValueArg<int> gridCVArg("", "gridSize", "Uniform grid size for cross-validation search", false, 20, "int");
		ValueArg<int> foldToComputeCVArg("", "computeFold", "Number of fold to iterate, default is 'fold' value", false, 10, "int");
		ValueArg<string> outFile2Arg("", "cvFileName", "Cross-validation output file name", false, "cv.txt", "cvFileName");

		//Bootstrap arguments
		SwitchArg doBootstrapArg("b", "bs", "Perform bootstrap estimation", false);
		ValueArg<string> bsOutFileArg("", "bsFileName", "Bootstrap output file name", false, "bs.txt", "bsFileName");
		ValueArg<int> replicatesArg("r", "replicates", "Number of bootstrap replicates", false, 200, "int");
		SwitchArg reportRawEstimatesArg("","raw", "Report the raw bootstrap estimates", false);

		cmd.add(gpuArg);
		cmd.add(betterGPUArg);
		cmd.add(toleranceArg);
		cmd.add(maxIterationsArg);
		cmd.add(hyperPriorArg);
		cmd.add(normalPriorArg);
		cmd.add(zhangOlesConvergenceArg);
		cmd.add(seedArg);

		cmd.add(doCVArg);
		cmd.add(lowerCVArg);
		cmd.add(upperCVArg);
		cmd.add(foldCVArg);
		cmd.add(gridCVArg);
		cmd.add(foldToComputeCVArg);
		cmd.add(outFile2Arg);

		cmd.add(doBootstrapArg);
		cmd.add(bsOutFileArg);
		cmd.add(replicatesArg);
		cmd.add(reportRawEstimatesArg);

		//tshaddox arguments
		cmd.add(hierarchyFileArg);
		cmd.add(classHierarchyVarianceArg);
		cmd.add(sigma2BetaArg);

		cmd.add(inFileArg);
		cmd.add(outFileArg);
		cmd.parse(argc, argv);

		if (gpuArg.getValue() > -1) {
			arguments.useGPU = true;
			arguments.deviceNumber = gpuArg.getValue();
		} else {
			arguments.useGPU = false;
		}
		arguments.useBetterGPU = betterGPUArg.isSet();

		arguments.inFileName = inFileArg.getValue();
		arguments.outFileName = outFileArg.getValue();
		arguments.hierarchyFileName = hierarchyFileArg.getValue(); //tshaddox argument
		arguments.classHierarchyVariance = classHierarchyVarianceArg.getValue(); //tshaddox argument
		arguments.sigma2Beta = sigma2BetaArg.getValue(); //tshaddox argument
		arguments.tolerance = toleranceArg.getValue();
		arguments.maxIterations = maxIterationsArg.getValue();
		arguments.hyperprior = hyperPriorArg.getValue();
		arguments.useNormalPrior = normalPriorArg.getValue();
		arguments.seed = seedArg.getValue();

		if (hyperPriorArg.isSet()) {
			arguments.hyperPriorSet = true;
		} else {
			arguments.hyperPriorSet = false;
		}

		if (zhangOlesConvergenceArg.isSet()) {
			arguments.convergenceType = ZHANG_OLES;
		} else {
			arguments.convergenceType = LANGE;
		}

		// Cross-validation
		arguments.doCrossValidation = doCVArg.isSet();
		if (arguments.doCrossValidation) {
			arguments.lowerLimit = lowerCVArg.getValue();
			arguments.upperLimit = upperCVArg.getValue();
			arguments.fold = foldCVArg.getValue();
			arguments.gridSteps = gridCVArg.getValue();
			if(foldToComputeCVArg.isSet()) {
				arguments.foldToCompute = foldToComputeCVArg.getValue();
			} else {
				arguments.foldToCompute = arguments.fold;
			}
			arguments.cvFileName = outFile2Arg.getValue();
			arguments.doFitAtOptimal = true;
		}

		// Bootstrap
		arguments.doBootstrap = doBootstrapArg.isSet();
		if (arguments.doBootstrap) {
			arguments.bsFileName = bsOutFileArg.getValue();
			arguments.replicates = replicatesArg.getValue();
			if (reportRawEstimatesArg.isSet()) {
				arguments.reportRawEstimates = true;
			} else {
				arguments.reportRawEstimates = false;
			}
		}

	} catch (ArgException &e) {
		cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
		exit(-1);
	}
}

double initializeModel(
		InputReader** reader,
		CyclicCoordinateDescent** ccd,
		CCDArguments &arguments, map<int, int> &drugIdToIndex) {
	
	//drugIdToIndex by tshaddox

	cout << "Running CCD (" <<
#ifdef DOUBLE_PRECISION
	"double"
#else
	"single"
#endif
	"-precision) ..." << endl;

	struct timeval time1, time2;
	gettimeofday(&time1, NULL);


	*reader = new InputReader(arguments.inFileName.c_str(), drugIdToIndex);

	(*reader)->classHierarchyVariance = arguments.classHierarchyVariance;
	(*reader)->sigma2Beta = arguments.sigma2Beta;

#ifdef CUDA
	if (arguments.useGPU) {
		if (arguments.useBetterGPU) {
			*ccd = new BetterGPU(arguments.deviceNumber, *reader);
//			*ccd = NULL;
		} else {
			*ccd = new GPUCyclicCoordinateDescent(arguments.deviceNumber, *reader);
		}
	} else {
#endif

	*ccd = new CyclicCoordinateDescent(*reader);

#ifdef CUDA
	}
#endif

	// Set prior from the command-line
	if (arguments.useNormalPrior) {
		(*ccd)->setPriorType(NORMAL);
	}
	if (arguments.hyperPriorSet) {
		(*ccd)->setHyperprior(arguments.hyperprior);
	}

	gettimeofday(&time2, NULL);
	double sec1 = calculateSeconds(time1, time2);

	cout << "Everything loaded and ready to run ..." << endl;
	
	return sec1;
}

double fitModel(CyclicCoordinateDescent *ccd, CCDArguments &arguments) {

	cout << "Using prior: " << ccd->getPriorInfo() << endl;

	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	ccd->update(arguments.maxIterations, arguments.convergenceType, arguments.tolerance);

	gettimeofday(&time2, NULL);

	ccd->logResults(arguments.outFileName.c_str());

	return calculateSeconds(time1, time2);
}

double runBoostrap(
		CyclicCoordinateDescent *ccd,
		InputReader *reader,
		CCDArguments &arguments) {
	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	BootstrapSelector selector(arguments.replicates, reader->getPidVectorSTL(),
			SUBJECT, arguments.seed);
	BootstrapDriver driver(arguments.replicates, reader);

	driver.drive(*ccd, selector, arguments);
	gettimeofday(&time2, NULL);

	driver.logResults(arguments);
	return calculateSeconds(time1, time2);
}

double runCrossValidation(CyclicCoordinateDescent *ccd, InputReader *reader,
		CCDArguments &arguments) {
	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	CrossValidationSelector selector(arguments.fold, reader->getPidVectorSTL(),
			SUBJECT, arguments.seed);
	CrossValidationDriver driver(arguments.gridSteps, arguments.lowerLimit, arguments.upperLimit);
/*
	driver.drive(*ccd, selector, arguments);
	driver.resetForOptimal(*ccd, selector, arguments);
	cout << "Post non-greedy Drive" << endl;
	cout << "sigma2Beta is " << ccd->sigma2Beta << endl;
	cout << "classHierarchyVariance is " << ccd->classHierarchyVariance << endl;
*/
	driver.greedyDrive(*ccd, selector, arguments);

	cout << "Post Greedy Drive" << endl;
	cout << "sigma2Beta is " << ccd->sigma2Beta << endl;
	cout << "classHierarchyVariance is " << ccd->classHierarchyVariance << endl;
	/*
	gettimeofday(&time2, NULL);

	driver.logResults(arguments);

	if (arguments.doFitAtOptimal) {
		std::cout << "Fitting model at optimal hyperparameter" << std::endl;
 		// Do full fit for optimal parameter
		driver.resetForOptimal(*ccd, selector, arguments);
		fitModel(ccd, arguments);
	}
	*/
	return calculateSeconds(time1, time2);
}

int main(int argc, char* argv[]) {

	CyclicCoordinateDescent* ccd = NULL;
	InputReader* reader = NULL;
	CCDArguments arguments;

	parseCommandLine(argc, argv, arguments);

	std::map<int, int> drugIdToIndex;

	double timeInitialize = initializeModel(&reader, &ccd, arguments, drugIdToIndex);

	// By tshaddox

	HierarchyReader* hierarchyReader = NULL;

	if (arguments.hierarchyFileName.c_str() == "noFileName") {
		cout << "No File!" << endl;
	}

	hierarchyReader = new HierarchyReader(arguments.hierarchyFileName.c_str(), drugIdToIndex, &ccd);

	// End tshaddox code



	double timeUpdate;
	if (arguments.doCrossValidation) {
		timeUpdate = runCrossValidation(ccd, reader, arguments);
	} else {
		timeUpdate = fitModel(ccd, arguments);
	}

	if (arguments.doBootstrap) {
		timeUpdate += runBoostrap(ccd, reader, arguments);
	}
		
	cout << "Load   duration: " << scientific << timeInitialize << endl;
	cout << "Update duration: " << scientific << timeUpdate << endl;
	
	if (ccd)
		delete ccd;
	if (reader)
		delete reader;


    return 0;
}
