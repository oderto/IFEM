// $Id$
//==============================================================================
//!
//! \file SIMoptions.h
//!
//! \date Feb 13 2012
//!
//! \author Knut Morten Okstad / SINTEF
//!
//! \brief Class for encapsulation of general simulation options.
//!
//==============================================================================

#ifndef SIM_OPTIONS_H_
#define SIM_OPTIONS_H_

#include "ASMenums.h"
#include <string>
#include <map>

class TiXmlElement;


/*!
  \brief Class for encapsulation of general simulation options.

  \details This class is supposed to contain all global simulation options
  that not already are stored in the model objects themselves. By keeping them
  independent of the model they can be initialized before instanciating
  the application-dependent model object(s). The class is equipped with some
  methods to initialize the options through the parsing of XML-tags.
*/

class SIMoptions
{
public:
  //! \brief The constructor initializes the default input options.
  SIMoptions();

  //! \brief Defines the linear equation solver to be used.
  void setLinearSolver(const std::string& eqsolver);

  //! \brief Parses a subelement of the \a discretization XML-tag.
  bool parseDiscretizationTag(const TiXmlElement* elem);
  //! \brief Parses a subelement of the \a eigensolver XML-tag.
  bool parseEigSolTag(const TiXmlElement* elem);
  //! \brief Parses a subelement of the \a resultoutput XML-tag.
  bool parseOutputTag(const TiXmlElement* elem);

  //! \brief Parses obsolete command-line arguments (backward compatibility).
  bool parseOldOptions(int argc, char** argv, int& i);

  //! \brief Returns whether HDF5 output is requested or not.
  bool dumpHDF5(char* defaultName);

public:
  ASM::Discretization discretization; //!< Spatial discretization option

  int nGauss[2]; //!< Gaussian quadrature rules

  int solver;          //!< The linear equation solver to use
  int num_threads_SLU; //!< Number of threads for SuperLU_MT

  // Eigenvalue solver options
  int    eig;   //!< Eigensolver method (1,...,5)
  int    nev;   //!< Number of eigenvalues/vectors
  int    ncv;   //!< Number of Arnoldi vectors
  double shift; //!< Eigenvalue shift

  // Output options
  int format;    //!< VTF-file format (-1=NONE, 0=ASCII, 1=BINARY)
  int nViz[3];   //!< Number of visualization points over each knot-span
  int saveInc;   //!< Number of increments between each result output
  double dtSave; //!< Time interval between each result output

  std::string hdf5; //!< Prefix for HDF5-file

  //! \brief Enum defining the available projection methods.
  enum ProjectionMethod { GLOBAL, DGL2, CGL2, SCR, VDSA, QUASI, LEASTSQ };
  //! \brief Projection method name mapping.
  typedef std::map<ProjectionMethod,std::string> ProjectionMap;

  ProjectionMap project; //!< The projection methods to use
};

#endif