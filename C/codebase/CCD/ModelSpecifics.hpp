/*
 * ModelSpecifics.hpp
 *
 *  Created on: Jul 13, 2012
 *      Author: msuchard
 */

#ifndef MODELSPECIFICS_HPP_
#define MODELSPECIFICS_HPP_

#include <cmath>
#include <cstdlib>

#include "ModelSpecifics.h"
#include "Iterators.h"

template <class BaseModel,typename WeightType>
ModelSpecifics<BaseModel,WeightType>::ModelSpecifics() : AbstractModelSpecifics(), BaseModel() {
	// TODO Memory allocation here
}

template <class BaseModel,typename WeightType>
ModelSpecifics<BaseModel,WeightType>::~ModelSpecifics() {
	// TODO Memory release here
}

template <class BaseModel,typename WeightType>
bool ModelSpecifics<BaseModel,WeightType>::allocateXjY(void) { return BaseModel::precomputeGradient; }

template <class BaseModel,typename WeightType>
bool ModelSpecifics<BaseModel,WeightType>::allocateXjX(void) { return BaseModel::precomputeHessian; }

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::setWeights(real* inWeights, bool useCrossValidation) {
	// Set K weights
	if (hKWeight.size() != K) {
		hKWeight.resize(K);
	}
	if (useCrossValidation) {
		for (int k = 0; k < K; ++k) {
			hKWeight[k] = inWeights[k];
		}
	} else {
		std::fill(hKWeight.begin(), hKWeight.end(), static_cast<WeightType>(1));
	}
	// Set N weights (these are the same for independent data models
	if (hNWeight.size() != N) {
		hNWeight.resize(N);
	}
	std::fill(hNWeight.begin(), hNWeight.end(), static_cast<WeightType>(0));
	for (int k = 0; k < K; ++k) {
		WeightType event = BaseModel::observationCount(hY[k])*hKWeight[k];
		incrementByGroup(hNWeight.data(), hPid, k, event);
	}
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeFixedTermsInGradientAndHessian(bool useCrossValidation) {
	if (allocateXjY()) {
		for (int j = 0; j < J; ++j) {
			hXjY[j] = 0;
			GenericIterator it(*hXI, j);

			if (useCrossValidation) {
				for (; it; ++it) {
					const int k = it.index();
					hXjY[j] += it.value() * hY[k] * hKWeight[k];
				}
			} else {
				for (; it; ++it) {
					const int k = it.index();
					hXjY[j] += it.value() * hY[k];
				}
			}
		}
	}

	if (allocateXjX()) {
		for (int j = 0; j < J; ++j) {
			hXjX[j] = 0;
			GenericIterator it(*hXI, j);

			if (useCrossValidation) {
				for (; it; ++it) {
					const int k = it.index();
					hXjX[j] += it.value() * it.value() * hKWeight[k];
				}
			} else {
				for (; it; ++it) {
					const int k = it.index();
					hXjX[j] += it.value() * it.value();
				}
			}
		}
	}
}

template <class BaseModel,typename WeightType>
double ModelSpecifics<BaseModel,WeightType>::getLogLikelihood(bool useCrossValidation) {

	real logLikelihood = static_cast<real>(0.0);
	if (useCrossValidation) {
		for (int i = 0; i < K; i++) {
			logLikelihood += BaseModel::logLikeNumeratorContrib(hY[i], hXBeta[i]) * hKWeight[i];
		}
	} else {
		for (int i = 0; i < K; i++) {
			logLikelihood += BaseModel::logLikeNumeratorContrib(hY[i], hXBeta[i]);
		}
	}

	if (BaseModel::likelihoodHasDenominator) { // Compile-time switch
		for (int i = 0; i < N; i++) {
			// Weights modified in computeNEvents()
			logLikelihood -= BaseModel::logLikeDenominatorContrib(hNWeight[i], denomPid[i]);
		}
	}
	return static_cast<double>(logLikelihood);
}

template <class BaseModel,typename WeightType>
double ModelSpecifics<BaseModel,WeightType>::getPredictiveLogLikelihood(real* weights) {
	real logLikelihood = static_cast<real>(0.0);

	for (int k = 0; k < K; ++k) {
		logLikelihood += BaseModel::logPredLikeContrib(hY[k], weights[k], hXBeta[k], denomPid, hPid, k);
	}

	return static_cast<double>(logLikelihood);
}

template <class BaseModel,typename WeightType>
std::vector<real> ModelSpecifics<BaseModel,WeightType>::getPredictiveEstimates(real* weights){

	std::vector<real> y(K,0.0);
	std::vector<real> xBeta(K,0.0);
	for(int j = 0; j < J; j++){
		GenericIterator it(*hXI, j);
		for(; it; ++it){
			const int k = it.index();
			xBeta[k] += it.value() * hBeta[j] * weights[k];
		}
	}
	for(int k = 0; k < K; k++){
		if(weights[k]){
			BaseModel::predictEstimate(y[k], xBeta[k]);
		}
	}
	return y;
}

// TODO The following function is an example of a double-dispatch, rewrite without need for virtual function
template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeGradientAndHessian(int index, double *ogradient,
		double *ohessian, bool useWeights) {
	// Run-time dispatch, so virtual call should not effect speed
	if (useWeights) {
		switch (hXI->getFormatType(index)) {
			case INDICATOR :
				computeGradientAndHessianImpl<IndicatorIterator>(index, ogradient, ohessian, weighted);
				break;
			case SPARSE :
				computeGradientAndHessianImpl<SparseIterator>(index, ogradient, ohessian, weighted);
				break;
			case DENSE :
				computeGradientAndHessianImpl<DenseIterator>(index, ogradient, ohessian, weighted);
				break;
		}
	} else {
		switch (hXI->getFormatType(index)) {
			case INDICATOR :
				computeGradientAndHessianImpl<IndicatorIterator>(index, ogradient, ohessian, unweighted);
				break;
			case SPARSE :
				computeGradientAndHessianImpl<SparseIterator>(index, ogradient, ohessian, unweighted);
				break;
			case DENSE :
				computeGradientAndHessianImpl<DenseIterator>(index, ogradient, ohessian, unweighted);
				break;
		}
	}
}

//incrementGradientAndHessian<SparseIterator>();

template <class BaseModel,typename WeightType> template <class IteratorType, class Weights>
void ModelSpecifics<BaseModel,WeightType>::computeGradientAndHessianImpl(int index, double *ogradient,
		double *ohessian, Weights w) {
	real gradient = 0;
	real hessian = 0;

	IteratorType it(*(*sparseIndices)[index], N); // TODO How to create with different constructor signatures?
	for (; it; ++it) {
		const int k = it.index();
		// Compile-time delegation
		BaseModel::incrementGradientAndHessian(it, w, // Signature-only, for iterator-type specialization
						&gradient, &hessian,
						numerPid[k], numerPid2[k], denomPid[k], hNWeight[k],
						it.value(), hXBeta[k], hY[k]
				); // When function is in-lined, compiler will only use necessary arguments
	}

	if (BaseModel::precomputeGradient) { // Compile-time switch
		gradient -= hXjY[index];
	}

	if (BaseModel::precomputeHessian) { // Compile-time switch
		hessian += static_cast<real>(2.0) * hXjX[index];
	}

	*ogradient = static_cast<double>(gradient);
	*ohessian = static_cast<double>(hessian);
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeNumeratorForGradient(int index) {
	// Run-time delegation
	switch (hXI->getFormatType(index)) {
		case INDICATOR : {
			IndicatorIterator it(*(*sparseIndices)[index]);
			for (; it; ++it) { // Only affected entries
				numerPid[it.index()] = static_cast<real>(0.0);
			}
			incrementNumeratorForGradientImpl<IndicatorIterator>(index);
			}
			break;
		case DENSE :
			zeroVector(numerPid, N);
			if (BaseModel::hasTwoNumeratorTerms) { // Compile-time switch
				zeroVector(numerPid2, N);
			}
			incrementNumeratorForGradientImpl<DenseIterator>(index);
			break;
		case SPARSE : {
			IndicatorIterator it(*(*sparseIndices)[index]);
			for (; it; ++it) { // Only affected entries
				numerPid[it.index()] = static_cast<real>(0.0);
				if (BaseModel::hasTwoNumeratorTerms) { // Compile-time switch
					numerPid2[it.index()] = static_cast<real>(0.0); // TODO Does this invalid the cache line too much?
				}
			}
			incrementNumeratorForGradientImpl<SparseIterator>(index); }
			break;
		default :
			// throw error
			exit(-1);
	}
}

template <class BaseModel,typename WeightType> template <class IteratorType>
void ModelSpecifics<BaseModel,WeightType>::incrementNumeratorForGradientImpl(int index) {
	IteratorType it(*hXI, index);
	for (; it; ++it) {
		const int k = it.index();
		incrementByGroup(numerPid, hPid, k,
				BaseModel::gradientNumeratorContrib(it.value(), offsExpXBeta[k], hXBeta[k], hY[k]));
		if (!IteratorType::isIndicator && BaseModel::hasTwoNumeratorTerms) {
			incrementByGroup(numerPid2, hPid, k,
					BaseModel::gradientNumerator2Contrib(it.value(), offsExpXBeta[k]));
		}
	}
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::updateXBeta(real realDelta, int index) {
	// Run-time dispatch to implementation depending on covariate FormatType
	switch(hXI->getFormatType(index)) {
		case INDICATOR :
			updateXBetaImpl<IndicatorIterator>(realDelta, index);
			break;
		case DENSE :
			updateXBetaImpl<DenseIterator>(realDelta, index);
			break;
		case SPARSE :
			updateXBetaImpl<SparseIterator>(realDelta, index);
			break;
		default :
			// throw error
			exit(-1);
	}
}

template <class BaseModel,typename WeightType> template <class IteratorType>
inline void ModelSpecifics<BaseModel,WeightType>::updateXBetaImpl(real realDelta, int index) {
	IteratorType it(*hXI, index);
	for (; it; ++it) {
		const int k = it.index();
		hXBeta[k] += realDelta * it.value();
		// Update denominators as well
		if (BaseModel::likelihoodHasDenominator) { // Compile-time switch
			real oldEntry = offsExpXBeta[k];
			real newEntry = offsExpXBeta[k] = BaseModel::getOffsExpXBeta(hOffs, hXBeta[k], hY[k], k);
			incrementByGroup(denomPid, hPid, k, (newEntry - oldEntry));
		}
	}
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeRemainingStatistics(void) {
	if (BaseModel::likelihoodHasDenominator) {
		fillVector(denomPid, N, BaseModel::getDenomNullValue());
		for (int k = 0; k < K; ++k) {
			offsExpXBeta[k] = BaseModel::getOffsExpXBeta(hOffs, hXBeta[k], hY[k], k);
			incrementByGroup(denomPid, hPid, k, offsExpXBeta[k]);
		}
	}
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::sortPid(bool useCrossValidation) {
/* For Cox model:
 *
 * We need to sort on hY[k], such that hY[hPid[k]] for k = 0,1,... are in increasing order.
 * Then, denom[k] is a subset of denom[k-1], and updating denom[k] implies updating demon[m]
 * for m = k,k-1,...,0.
 * NB: This should be an interesting update on the GPU, worthy of publication
 */
}

#endif /* MODELSPECIFICS_HPP_ */
