% Generated by roxygen2 (4.1.0): do not edit by hand
% Please edit documentation in R/ModelFit.R
\name{logLik.cyclopsFit}
\alias{logLik.cyclopsFit}
\title{Extract log-likelihood}
\usage{
\method{logLik}{cyclopsFit}(object, ...)
}
\arguments{
\item{object}{A Cyclops model fit object}

\item{...}{Additional arguments}
}
\description{
\code{logLik} returns the current log-likelihood of the fit in a Cyclops model fit object
}
\examples{
#Generate some simulated data:
sim <- simulateCyclopsData(nstrata = 1, nrows = 1000, ncovars = 2, eCovarsPerRow = 0.5,
                           model = "poisson")
cyclopsData <- convertToCyclopsData(sim$outcomes, sim$covariates, modelType = "pr",
                                    addIntercept = TRUE)

#Define the prior and control objects to use cross-validation for finding the
#optimal hyperparameter:
prior <- createPrior("laplace", exclude = 0, useCrossValidation = TRUE)
control <- createControl(cvType = "auto", noiseLevel = "quiet")

#Fit the model
fit <- fitCyclopsModel(cyclopsData,prior = prior, control = control)

#Find out what the optimal hyperparameter was:
getHyperParameter(fit)

#Extract the current log-likelihood, and coefficients
logLik(fit)
coef(fit)

#We can only retrieve the confidence interval for unregularized coefficients:
confint(fit, c(0))
}

