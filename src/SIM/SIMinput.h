// $Id$
//==============================================================================
//!
//! \file SIMinput.h
//!
//! \date Jun 1 2010
//!
//! \author Knut Morten Okstad / SINTEF
//!
//! \brief Base class for simulators with input parsing functionality.
//!
//==============================================================================

#ifndef _SIM_INPUT_H
#define _SIM_INPUT_H

#include <iostream>
#include <vector>

class TiXmlElement;


/*!
  \brief Base class for NURBS-based FEM simulators with input file parsing.
*/

class SIMinput
{
protected:
  //! \brief The default constructor initializes \a myPid and \a nProc.
  SIMinput();

public:
  //! \brief Empty destructor.
  virtual ~SIMinput() {}

  //! \brief Reads model data from the specified input file \a *fileName.
  virtual bool read(const char* fileName);

private:
  //! \brief Reads a flat text input file.
  bool readFlat(const char* fileName);
  //! \brief Reads an XML input file.
  bool readXML(const char* fileName);

protected:
  //! \brief Handles the parsing order for certain XML-tags.
  //! \param[in] base The base tag containing the elements to be prioritized
  //! \param[out] parsed Vector of XML-elements that was parsed
  //!
  //! \details Certain tags need to be parsed before others. This method takes
  //! care of this. It is called by the \a readXML method in order to read the
  //! top level tags in the required order. It can also be called by the
  //! application-specific SIM class prior to parsing its data blocks.
  //! In that case the \a getPrioritizedTags method should be reimplemented
  //! by the sub-class to take care of the application-specific tags.
  bool handlePriorityTags(const TiXmlElement* base,
			  std::vector<const TiXmlElement*>& parsed);

  //! \brief Returns a list of prioritized XML-tags.
  virtual const char** getPrioritizedTags() const { return NULL; }

public:
  //! \brief Parses a data section from an input stream.
  virtual bool parse(char* keyWord, std::istream& is) = 0;
  //! \brief Parses a data section from an XML element.
  virtual bool parse(const TiXmlElement* elem) = 0;

  static int msgLevel; //!< Controls the amount of console output during solving

protected:
  int myPid; //!< Processor ID in parallel simulations
  int nProc; //!< Number of processors in parallel simulations
};

#endif
