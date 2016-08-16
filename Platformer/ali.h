/* 
 * Copyright 2004, 2009, 2016 Roger Flores
 * 
 * This file is part of Ali.
 *
 * Ali is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ali is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Ali.  If not, see <http: *www.gnu.org/licenses/>.
 *
 */


/*! \file 
 *
 * This is the reference documentation for using Ali to input XML
 * documents. It is a simple to use C library that reduces the amount of code
 * needed to parse XML.
 * 
 * The header file ali.h declares the Ali API.  For more information 
 * and examples on using Ali, see the <a href="tutorial.html">Tutorial</a> 
 * and explore the provided test code.  */


/* This is an API to concisely read XML data into C data structures when
 * convienent for the C code.
 * 
 * The benefits are
 * 
 * 1. Simple and concise syntax.  Reading data from an XML element into a 
 * structure slot can be done in one line, without creating a specific callback 
 * function.  It's so easy!
 * 
 * 2. Powerful.  The format reading abilities of scanf are available for use.
 * 
 * 3. Minimum resources are needed.  If elements are read in the order found, 
 * then little memory is needed to skip elements.  If elements are strangely
 * ordered, then more memory is needed but the data is correctly read.
 * 
 * 
 * The disadvantages are
 * 
 * 1. Non validating.
 * 
 * 2. Not fully XML 1.0 compliant.  Nothing is violated, but some requirements
 * are not supported.
 * 
 * 3. No error location reporting.  This is not a design issue but an 
 * implementation time issue.
 * 
 * 4. Incorrect handling of XML formatting, including entities.
 * 
 * 5. No strong namespace support. */

#ifdef __cplusplus
extern "C"
{
#endif


/*! No options are needed to process the XML document. */
#define ALI_OPTION_NONE 0x00000000

/*! The XML must have an XML declaration.  Use this to ensure that the
 * document is well formed. */
#define ALI_OPTION_INPUT_XML_DECLARATION 0x00000002

/*! \brief Convert input from UTF-8 to ISO-8859-1.
 * 
 * Useful to read XML in code running in legacy environments. 
 *
 * Status of it's inclusion into the API is experimental until
 * experience shows enough apps justify permanent inclusion.*/
#ifndef ALI_OPTION_EXP_CONVERT_UTF8_TO_ISO_8859_1
   #define ALI_OPTION_EXP_CONVERT_UTF8_TO_ISO_8859_1 0x80000000
#endif


#ifndef ALI_ERROR_BASE
#define ALI_ERROR_BASE 0
#endif

/*! \brief There is no error.  
 * 
 * The last API completed successfully. */
#define ALI_ERROR_NONE (ALI_ERROR_BASE + 0)

/*! \brief A requested element or attribute is missing from the input. 
 *
 * Generally an app would want to reject the XML document. */
#define ALI_ERROR_TAG_MISSING (ALI_ERROR_BASE + 1)

/*! \brief The element is missing content.  
 *
 * The app requires the element to contain content like a number but it is empty. */
#define ALI_ERROR_CONTENT_MISSING (ALI_ERROR_BASE + 2)

/*! \brief The file to be opened for input is missing. 

   Try checking stdio's errno to determine the exact reason. */
#define ALI_ERROR_FILE_MISSING (ALI_ERROR_BASE + 3)

/*! \brief There is insufficient memory to read this XML document. */
#define ALI_ERROR_MEMORY_FAILURE (ALI_ERROR_BASE + 4)

/*! ali_in is requested to read data from an element that is no longer opened 
 * or never was. */
#define ALI_ERROR_ELEMENT_INVALID (ALI_ERROR_BASE + 5)

/*! The element or attributes name is invalid.  The allowed names are document at
 * http://www.w3.org/TR/REC-xml#NT-Stag and http://www.w3.org/TR/REC-xml#NT-Attribute */
#define ALI_ERROR_TAG_INVALID (ALI_ERROR_BASE + 6)

/*! The namespace parameter is not supported.  Put the entire qualified name in 
 *  the local part parameter. */
#define ALI_ERROR_NAMESPACE_INVALID (ALI_ERROR_BASE + 7)

/*! \brief The XML document ended before all data was read. */
#define ALI_ERROR_DATA_INCOMPLETE (ALI_ERROR_BASE + 8)

/*! \brief The XML document is encoded in a format not supported. 
 *
 * UTF-8, ISO-8859-X, and US-ASCII are supported.  Others like 
 * UTF-16 are not.
 */
#define ALI_ERROR_ENCODING_UNSUPPORTED (ALI_ERROR_BASE + 9)

/*! \brief The XML declaration, which starts the document, is
 * not well-formed. */
#define ALI_ERROR_XML_DECLARATION_INVALID (ALI_ERROR_BASE + 10)

/*! \brief The file is not an XML document, based on the missing
 * XML declaration when one is expected. */
#define ALI_ERROR_NOT_XML_DOCUMENT (ALI_ERROR_BASE + 11)

/*! \brief The XML Instruction is not known. 
 * 
 * XML Instructions, like "^e", are passed to ali_in.  The ali_in 
 * documentation lists all valid instructions. */
#define ALI_ERROR_UNKNOWN_XML_INSTRUCTION (ALI_ERROR_BASE + 12)

/*! \brief The tag may not be NULL. 
 * 
 * An XML Instruction, like "^e", was passed NULL for the tag name. */
#define ALI_ERROR_NULL_TAG (ALI_ERROR_BASE + 13)



/*! \brief The success status of inputting the XML document. 
 *
 * Will be ALI_ERROR_NONE until a problem occurs, at which point it gets one 
 * of the other values. */
    typedef int16_t ali_error;


/*! \brief An ID number for the namespace.  
 *
 * Used to specify the namespace for an element input by Ali. */
    typedef int16_t ali_namespace_ref;

/*! \brief An ID number for the element. 
 *
 * Used to confirm operating at the right element. */
    typedef int16_t ali_element_ref;

/*! \brief Where in the document to input information from. */
    typedef struct ali_element_info ali_element_info;

/*! \brief All information needed to read an XML document opened for input. */
    typedef struct ali_doc_info ali_doc_info;


/*! \brief Called to handle matching XML elements.  
 * 
 * The app provided function takes control of the input and should read all
 * important data within the element, storing it in app data structures as 
 * appropriate. It is one technique to read elements that occur multiple
 * times, and it can be used to factor code into smaller pieces. */

/* callback function to parse an element.
 * 
 * There where two models to pick from when designing these functions.  They 
 * call ali_in for each element to read.  One model is for ali_in to 
 * silently fail when an element is not readily available with the 
 * expectation of repeating this function until it is.  The other model 
 * is to keep reading elements until it is found.  The second model 
 * checks required elements more easily during the first pass, 
 * and so is the model adopted. */

    typedef void ali_element_function(ali_doc_info *doc,
        ali_element_ref element, void *data);

    extern ali_element_ref ali_open(ali_doc_info **doc, const char *file_name,
        uint32_t options, void *data);

    extern void ali_close(ali_doc_info *doc);

    extern ali_element_ref ali_in(ali_doc_info *doc, ali_element_ref element,
        const char *format, ...);

    extern bool ali_is_element_new(const ali_doc_info *doc,
        ali_element_ref element);

    extern bool ali_is_element_done(const ali_doc_info *doc,
        ali_element_ref element);

    extern ali_error ali_get_error(ali_doc_info *doc);

    extern void ali_set_error(ali_doc_info * doc, ali_error new_error);

#ifdef __cplusplus
}
#endif
