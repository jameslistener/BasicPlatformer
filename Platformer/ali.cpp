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
 * This is an API to read XML data into C data structures in a way convienent for the C code.
 * 
 * The benefits are
 * 
 * 1. Simple, concise, and familar syntax.  Reading data from an XML element 
 * into a structure slot can be done in one line, without creating 
 * a specific callback function.
 * 
 * 2. Powerful.  The format reading abilities of scanf are available 
 * for use.
 * 
 * 3. Minimum resources are needed.  If elements are read in the order 
 * found, then little memory is needed to skip elements.  If elements 
 * are strangely ordered, then more memory is needed but the data is 
 * correctly read.
 * 
 * 
 * The disadvantages are
 * 
 * 1. Non validating.
 * 
 * 2. Not fully XML 1.0 compliant.  Lack of DTD enforcement is 
 * the largest cause of this.
 * 
 * 3. No DOM support.  It is expected that the native data structures 
 * should provide adequate search and access mechanisms.
 * 
 * 4. No error location reporting.  This is not a design issue but an 
 * implementation time issue.
 * 
 * 5. No namespace support.
 * 
 * 
 * OVERVIEW
 * 
 * One way to understand this API is that it is similar to a 
 * fopen, fscanf, fclose sequence.  This is intentional because that 
 * model is simple and well understood.  The main difference is that 
 * ali_in (fscanf) takes a tag and reads from a matching element.  This means that 
 * the order of read operations need not match how the data is written. 
 * Discrepancies are resolved by a search (runtime hit), but not by 
 * a code or data change.
 * 
 * Data is read by read_one_markup().  Ideally the element read from the 
 * data is the element requested by the code and can be passed immeadiately. 
 * A storage system exists to store elements read out of order.  For 
 * developers concerned about reducing this overhead, a mode to gather and 
 * report statistics could be added.
 * 
 * Parsing of data is accomplished by passing the format string to fscanf.
 * 
 * Additional complexity comes from reading from different elements out of 
 * order.
 * 
 * Callers sometime need additional information.  For instance they may 
 * need to know if an element is being read for the first or last time to 
 * initialize or finalize a data structure.
 * 
 * If you think there's a bug there is a few things to consider.  The bug 
 * can be 1) in the document, 2) in your code, 3) in this code.  Viewing the 
 * document in a tool like Mozilla or Internet Explorer can help resolve the 
 * first case.  For the second, step through your code and pay attention to 
 * errors reported by Ali.  For bugs in this code, there are a few 
 * possibilities.  The reading can be at the wrong position.  Or the document 
 * could be read incorrectly (say a markup (like a comment) is not 
 * recognized as closed).  Or the code is not following the XML spec in a 
 * way needed for the document.  Or the formatting information could be 
 * wrong or passed incorrectly to sscanf_s, or the sscanf_s function is wrong.
 * 
 * 
 */

/* This file uses Doxygen to automatically generate documention. 
 * Please continue to use the funny commands in function comments. 
 * For more information see http://www.doxygen.org */

/* This file uses Indent to automatically format code. 
 * When adding code, make it look like the code above and below, 
 * or use Indent.  For more information see 
 * http://mysite.freeserve.com/indent/beautify.html */

/* TODO
 * 
 * - Support namespaces.  Currently namespaces should be specified with the element tag, and the
 * namespace should be left zero for don't care. 
 * Later namespaces should be fetched like
 * 
 * namespace = ali_get_namespace("xml");
 * 
 * and then namespace gets passed.  I like this better than always passing 
 * namespaces by string because it's faster (an integer compare versus a 
 * string compare).  And it avoids typos in namespaces.
 * 
 * - Add a function to describe the error 
 * ali_describe_error(&msg, &line, &position); 
 * line is currently not accurate 
 * position would be the difference between element->pos 
 * and the line(th) new line in doc->text. 
 * The goal is to enable user diagnostic of invalid data 
 * using a text editor.
 * 
 * - Tolerate non valid input 
 * - test chopped input for termination 
 * - test missing characters from input 
 * - test injected characters to input
 * 
 * - profiling shows that reading all elements has a substantial time hit. 
 * Roughly 50% more per element. This needs to be switched to read one 
 * element at a time, and to use the closing character position to start the 
 * following element (or else the skipping is not avoided)!
 * 
 * - Currently reading an element with markup inside requires a callback function.
 * Explore alternatives.  Perhaps passing a NULL function should just return a node.
 * There may be node closing issues.
 *
 * - Ali is not any better than SAX when reading unstructured XML like XHTML.  Often
 * passing in the XML unaltered is the best solution.  Consider adding "%r" to 
 * read all content of an element unaltered (raw).
 *
 * - Add "^*e" to read any element, "^**" to read anything, and so on.  Test by 
 * reading various documents.
 *
 * - explore including other documents.  Have something like
 * ali_include(doc, new_buffer, buffer_size).  These stack.  compare to XInclude.
 * 
 */
#include "ali_config.h"
#include "ali.h"
#include <iostream>

#ifdef __cplusplus
using namespace std;
#endif

/* Default size of the structure holding the elements.  Since reading the XML doc 
 * is depth first space is needed only the number of nested elements.  When space 
 * is exceed, the structure is doubled in size. */
#define ALI_CONFIG_DEFAULT_NESTED_ELEMENT_COUNT 8

#define tag_none 2
#define tag_element 3
#define tag_attribute 4
#define tag_comment 5
#define tag_instruction 6



/* There is a need to safely read input when available or error when input is 
   not available.  The desire is to avoid adding code everywhere to check this.
   The solution is to have a type define with suitable properties. It only has
   the operaters deref(), =, and +.  deref is only valid if the value is within
   the input.  It returns a value when valid, and throws an exception when not.
   This would be much nicer in c++!
 */
typedef const char *ali_char;



/* limit for number of elements inside an element that are not read, but 
 * need to be remembered incase they are wanted later. */
#define ali_element_max 16      /*!< \internal */
#define ali_element_none ali_element_max    /*!< \internal */

struct ali_element_info
{
    /* Sort of an ID number for the node.  Used to confirm operating at the 
     * right spot. */
    ali_element_ref element;
    
    /* Element that contained this element. Used to generate paths. */
    ali_element_ref parent;
    
    /* True once past the start tag.  Used to determine if reading
     * attributes or content. */
    bool start_tag_closed;
    
    /* True indicates that the elements have been read.  Eventually this
     * may be a flag to indicate that no more elements are available, which 
     * will be useful when switching to a read only when needed model. */
    bool elements_read;
    
    /* True when any data is used in a element by the callback functions.
     * Used to indicate that repeating a callback to handle unused data
     * could be productive. */
    bool data_used;
    
    /* Data was asked for but not available.  At the end of a parse
     * function, if there is data and data was asked for but not available, 
     * then reenter the callback because now the data may be available. */
    bool data_unavailable;
    
    /* True the first time an element is read and passed to a callback.
     * False every time thereafter.  This is queried using ali_new_element
     * by the callbacks to determine if they should be creating new data
     * structures or adding to existing ones. */
    bool new_element;
    
    /* name and length of the element's start tag.  Child markup is looked
     * for after it. */
    ali_char name;
    int32_t length;
    int8_t element_type;
    
    /* Number of markup available. */
    int16_t count;
    
    /* Note that the name + length allows reading of the contents of any
     * element at anytime, allowing random access. */
    ali_char markup_name[ali_element_max];
    int16_t markup_length[ali_element_max];
    int8_t markup_type[ali_element_max];    /* element or attribute */
    
    /* the last markup read */
    int8_t last_markup_read;
    int8_t type_of_last_markup_read;    /* type of last markup read */
    ali_char last_markup_name;  /* allows for reading the next markup so
                                 * that out of order markup usage doesn't
                                 * confuse the reading.
                                 * 
                                 * It can be at three different positions,
                                 * controlled by type_of_last_markup_read.
                                 * It can be at the start of an attribute
                                 * name, the start of an element name, or
                                 * in the content of owning element (in
                                 * other words after the last element's end 
                                 * tag). */
    int32_t last_markup_name_length;
                                        
    /* data input position.  Kept per element to allow random access. */
    ali_char pos;
    int32_t line_number;
    
    /* There can be errors from data operations.  These are stored in the
     * doc structure, so a pointer to it is convienent.  A try/catch/throw
     * mechanism would eliminate this. */
    struct ali_doc_info *doc;
                                        
};

typedef enum
{
    endian_unknown,
    endian_native,
    endian_swapped
} endian_type;

typedef enum
{
    encoding_unknown, /* Use when an encoding is specified but not known. */
    encoding_UTF_8, /* default */
    encoding_ISO_8859_N,
    encoding_US_ASCII,
    encoding_UTF_16,
    encoding_UTF_16_SWAPPED,    /* not native endian. */
    encoding_UTF_32,
    encoding_UTF_32_SWAPPED,
    encoding_EBCDIC
} encoding_type;

#define encoding_supported(x) ((x) <= encoding_US_ASCII)

struct ali_doc_info
{
    FILE *file_in;
    ali_char text;
    ali_error error;
    /* Used to determine another unique ID. */
    ali_element_ref next_element;
    uint32_t size;
    ali_char last_valid_char;
    ali_element_info *current_element;
    void *data;
    endian_type endian;
    encoding_type encoding;
    jmp_buf environment;

    bool standalone;
    bool standalone_declared;
    /* The options requested to input the XML document. */
    uint32_t options;
        
    ali_element_info *elements;
    uint16_t elements_size;
    uint16_t elements_count;
};


/* These UTF-8 macros exist only because the code checks that tag and attribute
 * names are well-formed. */
/* Is this char the leader of a multi byte sequence? */
#define IS_UTF_8_LEADER(c) ((uint8_t) ((c) - 0xc0) < 0x3e)

/* Is this char a trailer of a multi byte sequence? */
#define IS_UTF_8_TRAILER(c) (((c) & 0xc0) == 0x80)

/* Is this a single byte char? */
#define IS_UTF_8_SINGLE(c) (((c) & 0x80) == 0)

/* Prepare the lead byte for combining with the trail bytes to form 
 * a 21 bit code point. */
/*#define PREPARE_LEAD_BYTE(c) ((c) = (c) & (0x07 | (((c) & 0x30) >> 1)))    Not quite right*/
#define PREPARE_LEAD_BYTE(c) ((c) = ((c) & (((c) < 0xdf) ? 0x1f : ((c) < 0xdf) ? 0x0f : 0x07)))


/* Returns the char at c, or sets ALI_ERROR_DATA_INCOMPLETE if c is invalid, and jumps to the
 * end of ali_in().
 * 
 * This uses the setjmp/longjump pair.  It greatly simplies the code and reduces 
 * overhead at the cost of requiring a setjmp implementation.  Hopefully this is 
 * a good tradeoff.  Nesting is not needed, so a try/catch C implementation was 
 * not used.  But this issue is a major reason to switch to C++. */
static uint8_t
deref(
   ali_doc_info * doc,
   ali_char c)
{
   if (c > doc->last_valid_char)
   {
      doc->error = ALI_ERROR_DATA_INCOMPLETE;
      longjmp(doc->environment, ALI_ERROR_DATA_INCOMPLETE);
   }
   return *c;
}


static int
skip_whitespace(
   ali_element_info * element)
{
   ali_char c;
   uint8_t safe_c;
   int last_char_was_linefeed = false;
   int found_whitespace;

   assert(element != NULL);

   c = element->pos;

   for (;;)
   {
      safe_c = deref(element->doc, c);
      if (safe_c == 0x0d)
      {
         if (!last_char_was_linefeed)
            element->line_number++;

         c++;
      }
      else if (safe_c == 0x0a)
      {
         last_char_was_linefeed = true;
         element->line_number++;

         c++;
      }
      else if (safe_c == ' ' || safe_c == '\t')
      {
         last_char_was_linefeed = true;

         c++;
      }
      else
      {
         break;
      }
   }

   found_whitespace = element->pos != c;

   element->pos = c;

   return found_whitespace;
}


/* skip dtd */
/* handle DTD DOCTYPE.  Skip the DTD.  This could be a separate
* function.
*
* We could add code to parse the DTD.  I'm also thinking about 
* letting external code parse the DTD.  This avoids bloating 
* Ali with a questionable feature.  Or make it a compile option.
* The same scheme can handle schemas too. But for now, skip it
* instead of rejecting the document. */
        
static int
skip_dtd(
   ali_element_info * element)
{
   ali_char c;
   int found_dtd;

   assert(element != NULL);

   c = element->pos;

   if (strncmp(element->pos, "<!DOCTYPE", 9) == 0)
   {
      c += 9;
      
      /*! \DOLATER handle all DTD formats. */
      while (deref(element->doc, c) != 0 && strncmp(c, ">", 1) != 0)
      {
         c++;
      }
   }

   found_dtd = element->pos != c;
   element->pos = c;
   return found_dtd;
}


static void
skip_to_end_of_comment(
   ali_element_info * element)
{
   uint8_t safe_c;

   /* the tag is a comment.  Skip to the end of the comment.
    * http://www.w3.org/TR/REC-xml#NT-Comment */
   safe_c = deref(element->doc, element->pos);
   while (safe_c != '\0' &&
      (element->pos[0] != '-' || element->pos[1] != '-' || element->pos[2] != '>'))
   {
      element->pos++;
      safe_c = deref(element->doc, element->pos);
   }

   element->pos += 3;        /* skip terminator */
}


static void
skip_to_end_of_processing_instruction(
   ali_element_info * element)
{
   uint8_t safe_c;

   /* the tag is a processing instruction.  Skip to the end of the instruction.
    * http://www.w3.org/TR/REC-xml#NT-PI */
   safe_c = deref(element->doc, element->pos);
   while (safe_c != '\0' &&
      (safe_c != '?' || element->pos[1] != '>'))
   {
      element->pos++;
      safe_c = deref(element->doc, element->pos);
   }

   element->pos += 2;        /* skip terminator */
}


static void
skip_element_tag(
   ali_element_info * element,
   int16_t * length             /* if not NULL, set to length of tag. */
   )
{
   ali_char c;
   uint32_t safe_c;
   uint8_t next_c;
   bool trailer;
   bool name_start = true;
   int16_t tag_length;

   assert(element != NULL);

   c = element->pos;
   /* http://www.w3.org/TR/REC-xml#NT-Name
    * 
    * This code is currently a bit looser than spec. 
    * Specifically it should restrict the first letter more. 
    * It also doesn't do double byte chars. */
   while ((safe_c = deref(element->doc, c)) != '\0')
   {
      if (element->doc->encoding == encoding_UTF_8 && IS_UTF_8_LEADER(safe_c))
      {
          /* Read the complete multi byte sequence before checking it. */
          PREPARE_LEAD_BYTE(safe_c);
          do
          {
              next_c = deref(element->doc, c + 1);

              trailer = IS_UTF_8_TRAILER(next_c);
              if (trailer)
              {
                  safe_c = (safe_c << 6) | (next_c & 0x3f);
                  c++;
              }
          } while (trailer);
      }

      /* http://www.w3.org/TR/REC-xml#NT-Letter */
      /* Check for single byte values first, then multi byte. */
       
      /* http://www.w3.org/TR/REC-xml#NT-BaseChar */
      if ((0x41 <= safe_c && safe_c <= 0x5a) || (0x61 <= safe_c && safe_c <= 0x7a) ||
         (0xc0 <= safe_c && safe_c <= 0xd6) || (0xd8 <= safe_c && safe_c <= 0xf6) || (0xf8 <= safe_c
            && safe_c <= 0xff) ||
         /* http://www.w3.org/TR/REC-xml#NT-NameChar */
         '_' == safe_c || ':' == safe_c ||

         (!name_start &&
             ('.' == safe_c || '-' == safe_c ||
              /* http://www.w3.org/TR/REC-xml#NT-Digit */
              (0x30 <= safe_c && safe_c <= 0x39))) ||
         
         /* Now check multi byte code point values. 
          * This doesn't really help the code footprint! Consider an array? */

         /* http://www.w3.org/TR/REC-xml#NT-Ideographic */
         (0x4E00 <= safe_c && safe_c <= 0x9FA5) || 0x3007 == safe_c || (0x3021 <= safe_c && safe_c <= 0x3029) ||


         /* http://www.w3.org/TR/REC-xml#NT-BaseChar (multibyte) */
         (0x0100 <= safe_c && safe_c <= 0x0131) || (0x0134 <= safe_c && safe_c <= 0x013E) || 
         (0x0141 <= safe_c && safe_c <= 0x0148) || (0x014A <= safe_c && safe_c <= 0x017E) || 
         (0x0180 <= safe_c && safe_c <= 0x01C3) || (0x01CD <= safe_c && safe_c <= 0x01F0) || 
         (0x01F4 <= safe_c && safe_c <= 0x01F5) || (0x01FA <= safe_c && safe_c <= 0x0217) || 
         (0x0250 <= safe_c && safe_c <= 0x02A8) || (0x02BB <= safe_c && safe_c <= 0x02C1) || 0x0386 == safe_c || 
         (0x0388 <= safe_c && safe_c <= 0x038A) || 0x038C == safe_c || 
         (0x038E <= safe_c && safe_c <= 0x03A1) || (0x03A3 <= safe_c && safe_c <= 0x03CE) || 
         (0x03D0 <= safe_c && safe_c <= 0x03D6) || 0x03DA == safe_c || 0x03DC == safe_c || 0x03DE == safe_c || 0x03E0 == safe_c || 
         (0x03E2 <= safe_c && safe_c <= 0x03F3) || (0x0401 <= safe_c && safe_c <= 0x040C) || 
         (0x040E <= safe_c && safe_c <= 0x044F) || 
         (0x0451 <= safe_c && safe_c <= 0x045C) || (0x045E <= safe_c && safe_c <= 0x0481) || 
         (0x0490 <= safe_c && safe_c <= 0x04C4) || (0x04C7 <= safe_c && safe_c <= 0x04C8) || 
         (0x04CB <= safe_c && safe_c <= 0x04CC) || (0x04D0 <= safe_c && safe_c <= 0x04EB) || 
         (0x04EE <= safe_c && safe_c <= 0x04F5) || (0x04F8 <= safe_c && safe_c <= 0x04F9) || 
         (0x0531 <= safe_c && safe_c <= 0x0556) || 0x0559 == safe_c || (0x0561 <= safe_c && safe_c <= 0x0586) || (0x05D0 <= safe_c && safe_c <= 0x05EA) || 
         (0x05F0 <= safe_c && safe_c <= 0x05F2) || (0x0621 <= safe_c && safe_c <= 0x063A) || 
         (0x0641 <= safe_c && safe_c <= 0x064A) || (0x0671 <= safe_c && safe_c <= 0x06B7) || 
         (0x06BA <= safe_c && safe_c <= 0x06BE) || (0x06C0 <= safe_c && safe_c <= 0x06CE) || 
         (0x06D0 <= safe_c && safe_c <= 0x06D3) || 0x06D5 == safe_c || (0x06E5 <= safe_c && safe_c <= 0x06E6) || 
         (0x0905 <= safe_c && safe_c <= 0x0939) || 0x093D == safe_c || (0x0958 <= safe_c && safe_c <= 0x0961) || (0x0985 <= safe_c && safe_c <= 0x098C) || 
         (0x098F <= safe_c && safe_c <= 0x0990) || (0x0993 <= safe_c && safe_c <= 0x09A8) || 
         (0x09AA <= safe_c && safe_c <= 0x09B0) || 0x09B2 == safe_c || (0x09B6 <= safe_c && safe_c <= 0x09B9) || (0x09DC <= safe_c && safe_c <= 0x09DD) || 
         (0x09DF <= safe_c && safe_c <= 0x09E1) || (0x09F0 <= safe_c && safe_c <= 0x09F1) || 
         (0x0A05 <= safe_c && safe_c <= 0x0A0A) || (0x0A0F <= safe_c && safe_c <= 0x0A10) || 
         (0x0A13 <= safe_c && safe_c <= 0x0A28) || (0x0A2A <= safe_c && safe_c <= 0x0A30) || 
         (0x0A32 <= safe_c && safe_c <= 0x0A33) || (0x0A35 <= safe_c && safe_c <= 0x0A36) || 
         (0x0A38 <= safe_c && safe_c <= 0x0A39) || (0x0A59 <= safe_c && safe_c <= 0x0A5C) || 0x0A5E == safe_c || (0x0A72 <= safe_c && safe_c <= 0x0A74) || 
         (0x0A85 <= safe_c && safe_c <= 0x0A8B) || 0x0A8D == safe_c || (0x0A8F <= safe_c && safe_c <= 0x0A91) || 
         (0x0A93 <= safe_c && safe_c <= 0x0AA8) || (0x0AAA <= safe_c && safe_c <= 0x0AB0) || 
         (0x0AB2 <= safe_c && safe_c <= 0x0AB3) || 
         (0x0AB5 <= safe_c && safe_c <= 0x0AB9) || 0x0ABD == safe_c || 0x0AE0 == safe_c || (0x0B05 <= safe_c && safe_c <= 0x0B0C) || 
         (0x0B0F <= safe_c && safe_c <= 0x0B10) || (0x0B13 <= safe_c && safe_c <= 0x0B28) || 
         (0x0B2A <= safe_c && safe_c <= 0x0B30) || 
         (0x0B32 <= safe_c && safe_c <= 0x0B33) || (0x0B36 <= safe_c && safe_c <= 0x0B39) || 0x0B3D == safe_c || 
         (0x0B5C <= safe_c && safe_c <= 0x0B5D) || (0x0B5F <= safe_c && safe_c <= 0x0B61) || (0x0B85 <= safe_c && safe_c <= 0x0B8A) || 
         (0x0B8E <= safe_c && safe_c <= 0x0B90) || (0x0B92 <= safe_c && safe_c <= 0x0B95) || 
         (0x0B99 <= safe_c && safe_c <= 0x0B9A) || 0x0B9C == safe_c || (0x0B9E <= safe_c && safe_c <= 0x0B9F) || (0x0BA3 <= safe_c && safe_c <= 0x0BA4) || 
         (0x0BA8 <= safe_c && safe_c <= 0x0BAA) || (0x0BAE <= safe_c && safe_c <= 0x0BB5) || 
         (0x0BB7 <= safe_c && safe_c <= 0x0BB9) || (0x0C05 <= safe_c && safe_c <= 0x0C0C) || 
         (0x0C0E <= safe_c && safe_c <= 0x0C10) || (0x0C12 <= safe_c && safe_c <= 0x0C28) || 
         (0x0C2A <= safe_c && safe_c <= 0x0C33) || (0x0C35 <= safe_c && safe_c <= 0x0C39) || 
         (0x0C60 <= safe_c && safe_c <= 0x0C61) || (0x0C85 <= safe_c && safe_c <= 0x0C8C) || 
         (0x0C8E <= safe_c && safe_c <= 0x0C90) || (0x0C92 <= safe_c && safe_c <= 0x0CA8) || 
         (0x0CAA <= safe_c && safe_c <= 0x0CB3) || (0x0CB5 <= safe_c && safe_c <= 0x0CB9) || 0x0CDE == safe_c || 
         (0x0CE0 <= safe_c && safe_c <= 0x0CE1) || (0x0D05 <= safe_c && safe_c <= 0x0D0C) || (0x0D0E <= safe_c && safe_c <= 0x0D10) || 
         (0x0D12 <= safe_c && safe_c <= 0x0D28) || (0x0D2A <= safe_c && safe_c <= 0x0D39) || 
         (0x0D60 <= safe_c && safe_c <= 0x0D61) || (0x0E01 <= safe_c && safe_c <= 0x0E2E) || 0x0E30 == safe_c || 
         (0x0E32 <= safe_c && safe_c <= 0x0E33) || 
         (0x0E40 <= safe_c && safe_c <= 0x0E45) || (0x0E81 <= safe_c && safe_c <= 0x0E82) || 0x0E84 == safe_c || 
         (0x0E87 <= safe_c && safe_c <= 0x0E88) || 0x0E8A == safe_c || 0x0E8D == safe_c || (0x0E94 <= safe_c && safe_c <= 0x0E97) || 
         (0x0E99 <= safe_c && safe_c <= 0x0E9F) || (0x0EA1 <= safe_c && safe_c <= 0x0EA3) || 0x0EA5 == safe_c || 0x0EA7 == safe_c || 
         (0x0EAA <= safe_c && safe_c <= 0x0EAB) || (0x0EAD <= safe_c && safe_c <= 0x0EAE) || 0x0EB0 == safe_c || 
         (0x0EB2 <= safe_c && safe_c <= 0x0EB3) || 0x0EBD == safe_c || (0x0EC0 <= safe_c && safe_c <= 0x0EC4) || 
         (0x0F40 <= safe_c && safe_c <= 0x0F47) || (0x0F49 <= safe_c && safe_c <= 0x0F69) || (0x10A0 <= safe_c && safe_c <= 0x10C5) || 
         (0x10D0 <= safe_c && safe_c <= 0x10F6) || 0x1100 == safe_c || (0x1102 <= safe_c && safe_c <= 0x1103) || 
         (0x1105 <= safe_c && safe_c <= 0x1107) || 0x1109 == safe_c || (0x110B <= safe_c && safe_c <= 0x110C) || 
         (0x110E <= safe_c && safe_c <= 0x1112) || 0x113C == safe_c || 0x113E == safe_c || 0x1140 == safe_c || 0x114C == safe_c || 0x114E == safe_c || 0x1150 == safe_c || 
         (0x1154 <= safe_c && safe_c <= 0x1155) || 0x1159 == safe_c || 
         (0x115F <= safe_c && safe_c <= 0x1161) || 0x1163 == safe_c || 0x1165 == safe_c || 0x1167 == safe_c || 0x1169 == safe_c || (0x116D <= safe_c && safe_c <= 0x116E) || 
         (0x1172 <= safe_c && safe_c <= 0x1173) || 0x1175 == safe_c || 0x119E == safe_c || 0x11A8 == safe_c || 0x11AB == safe_c || 
         (0x11AE <= safe_c && safe_c <= 0x11AF) || (0x11B7 <= safe_c && safe_c <= 0x11B8) || 0x11BA == safe_c || 
         (0x11BC <= safe_c && safe_c <= 0x11C2) || 0x11EB == safe_c || 0x11F0 == safe_c || 0x11F9 == safe_c || (0x1E00 <= safe_c && safe_c <= 0x1E9B) || 
         (0x1EA0 <= safe_c && safe_c <= 0x1EF9) || (0x1F00 <= safe_c && safe_c <= 0x1F15) || 
         (0x1F18 <= safe_c && safe_c <= 0x1F1D) || (0x1F20 <= safe_c && safe_c <= 0x1F45) || 
         (0x1F48 <= safe_c && safe_c <= 0x1F4D) || (0x1F50 <= safe_c && safe_c <= 0x1F57) || 0x1F59 == safe_c || 0x1F5B == safe_c || 0x1F5D == safe_c || 
         (0x1F5F <= safe_c && safe_c <= 0x1F7D) || (0x1F80 <= safe_c && safe_c <= 0x1FB4) || 
         (0x1FB6 <= safe_c && safe_c <= 0x1FBC) || 0x1FBE == safe_c || (0x1FC2 <= safe_c && safe_c <= 0x1FC4) || (0x1FC6 <= safe_c && safe_c <= 0x1FCC) || 
         (0x1FD0 <= safe_c && safe_c <= 0x1FD3) || (0x1FD6 <= safe_c && safe_c <= 0x1FDB) || 
         (0x1FE0 <= safe_c && safe_c <= 0x1FEC) || (0x1FF2 <= safe_c && safe_c <= 0x1FF4) || 
         (0x1FF6 <= safe_c && safe_c <= 0x1FFC) || 0x2126 == safe_c || 
         (0x212A <= safe_c && safe_c <= 0x212B) || 0x212E == safe_c || (0x2180 <= safe_c && safe_c <= 0x2182) || 
         (0x3041 <= safe_c && safe_c <= 0x3094) || (0x30A1 <= safe_c && safe_c <= 0x30FA) || 
         (0x3105 <= safe_c && safe_c <= 0x312C) || (0xAC00 <= safe_c && safe_c <= 0xD7A3) ||






         (!name_start &&
             ('.' == safe_c || '-' == safe_c ||
              /* http://www.w3.org/TR/REC-xml#NT-Digit */
             (0x0660 <= safe_c && safe_c <= 0x0669) ||
             (0x06F0 <= safe_c && safe_c <= 0x06F9) || 
             (0x0966 <= safe_c && safe_c <= 0x096F) || 
             (0x09E6 <= safe_c && safe_c <= 0x09EF) || 
             (0x0A66 <= safe_c && safe_c <= 0x0A6F) || 
             (0x0AE6 <= safe_c && safe_c <= 0x0AEF) || 
             (0x0B66 <= safe_c && safe_c <= 0x0B6F) || 
             (0x0BE7 <= safe_c && safe_c <= 0x0BEF) || 
             (0x0C66 <= safe_c && safe_c <= 0x0C6F) || 
             (0x0CE6 <= safe_c && safe_c <= 0x0CEF) || 
             (0x0D66 <= safe_c && safe_c <= 0x0D6F) || 
             (0x0E50 <= safe_c && safe_c <= 0x0E59) || 
             (0x0ED0 <= safe_c && safe_c <= 0x0ED9) || 
             (0x0F20 <= safe_c && safe_c <= 0x0F29) ||

             /* http://www.w3.org/TR/REC-xml#NT-Extender */
             0x00B7 == safe_c || 0x02D0 == safe_c || 0x02D1 == safe_c || 0x0387 == safe_c || 
             0x0640 == safe_c || 0x0E46 == safe_c || 0x0EC6 == safe_c || 0x3005 == safe_c || 
             (0x3031 <= safe_c && safe_c <= 0x3035) || 
             (0x309D <= safe_c && safe_c <= 0x309E) || 
             (0x30FC <= safe_c && safe_c <= 0x30FE) ||

             /* http://www.w3.org/TR/REC-xml#NT-CombiningChar */
             (0x0300 <= safe_c && safe_c <= 0x0345) || (0x0360 <= safe_c && safe_c <= 0x0361) || (0x0483 <= safe_c && safe_c <= 0x0486) || 
             (0x0591 <= safe_c && safe_c <= 0x05A1) || (0x05A3 <= safe_c && safe_c <= 0x05B9) || 
             (0x05BB <= safe_c && safe_c <= 0x05BD) || 0x05BF == safe_c || (0x05C1 <= safe_c && safe_c <= 0x05C2) || 0x05C4 == safe_c || 
             (0x064B <= safe_c && safe_c <= 0x0652) || 0x0670 == safe_c || (0x06D6 <= safe_c && safe_c <= 0x06DC) || 
             (0x06DD <= safe_c && safe_c <= 0x06DF) || (0x06E0 <= safe_c && safe_c <= 0x06E4) || (0x06E7 <= safe_c && safe_c <= 0x06E8) || 
             (0x06EA <= safe_c && safe_c <= 0x06ED) || (0x0901 <= safe_c && safe_c <= 0x0903) || 0x093C == safe_c || 
             (0x093E <= safe_c && safe_c <= 0x094C) || 0x094D == safe_c || (0x0951 <= safe_c && safe_c <= 0x0954) || 
             (0x0962 <= safe_c && safe_c <= 0x0963) || (0x0981 <= safe_c && safe_c <= 0x0983) || 
             0x09BC == safe_c || 0x09BE == safe_c || 0x09BF == safe_c || (0x09C0 <= safe_c && safe_c <= 0x09C4) || 
             (0x09C7 <= safe_c && safe_c <= 0x09C8) || (0x09CB <= safe_c && safe_c <= 0x09CD) || 0x09D7 == safe_c || 
             (0x09E2 <= safe_c && safe_c <= 0x09E3) || 0x0A02 == safe_c || 0x0A3C == safe_c || 
             0x0A3E == safe_c || 0x0A3F == safe_c || (0x0A40 <= safe_c && safe_c <= 0x0A42) || 
             (0x0A47 <= safe_c && safe_c <= 0x0A48) || (0x0A4B <= safe_c && safe_c <= 0x0A4D) || 
             (0x0A70 <= safe_c && safe_c <= 0x0A71) || (0x0A81 <= safe_c && safe_c <= 0x0A83) || 0x0ABC == safe_c || 
             (0x0ABE <= safe_c && safe_c <= 0x0AC5) || (0x0AC7 <= safe_c && safe_c <= 0x0AC9) || 
             (0x0ACB <= safe_c && safe_c <= 0x0ACD) || (0x0B01 <= safe_c && safe_c <= 0x0B03) || 0x0B3C == safe_c || 
             (0x0B3E <= safe_c && safe_c <= 0x0B43) || (0x0B47 <= safe_c && safe_c <= 0x0B48) || 
             (0x0B4B <= safe_c && safe_c <= 0x0B4D) || (0x0B56 <= safe_c && safe_c <= 0x0B57) || 
             (0x0B82 <= safe_c && safe_c <= 0x0B83) || (0x0BBE <= safe_c && safe_c <= 0x0BC2) || 
             (0x0BC6 <= safe_c && safe_c <= 0x0BC8) || (0x0BCA <= safe_c && safe_c <= 0x0BCD) || 0x0BD7 == safe_c || 
             (0x0C01 <= safe_c && safe_c <= 0x0C03) || (0x0C3E <= safe_c && safe_c <= 0x0C44) || 
             (0x0C46 <= safe_c && safe_c <= 0x0C48) || (0x0C4A <= safe_c && safe_c <= 0x0C4D) || 
             (0x0C55 <= safe_c && safe_c <= 0x0C56) || (0x0C82 <= safe_c && safe_c <= 0x0C83) || 
             (0x0CBE <= safe_c && safe_c <= 0x0CC4) || (0x0CC6 <= safe_c && safe_c <= 0x0CC8) || 
             (0x0CCA <= safe_c && safe_c <= 0x0CCD) || (0x0CD5 <= safe_c && safe_c <= 0x0CD6) || 
             (0x0D02 <= safe_c && safe_c <= 0x0D03) || (0x0D3E <= safe_c && safe_c <= 0x0D43) || 
             (0x0D46 <= safe_c && safe_c <= 0x0D48) || (0x0D4A <= safe_c && safe_c <= 0x0D4D) || 0x0D57 == safe_c || 0x0E31 == safe_c || 
             (0x0E34 <= safe_c && safe_c <= 0x0E3A) || (0x0E47 <= safe_c && safe_c <= 0x0E4E) || 0x0EB1 == safe_c || 
             (0x0EB4 <= safe_c && safe_c <= 0x0EB9) || 
             (0x0EBB <= safe_c && safe_c <= 0x0EBC) || (0x0EC8 <= safe_c && safe_c <= 0x0ECD) || 
             (0x0F18 <= safe_c && safe_c <= 0x0F19) || 0x0F35 == safe_c || 0x0F37 == safe_c || 
             0x0F39 == safe_c || 0x0F3E == safe_c || 0x0F3F == safe_c || (0x0F71 <= safe_c && safe_c <= 0x0F84) || 
             (0x0F86 <= safe_c && safe_c <= 0x0F8B) || (0x0F90 <= safe_c && safe_c <= 0x0F95) || 0x0F97 == safe_c || 
             (0x0F99 <= safe_c && safe_c <= 0x0FAD) || (0x0FB1 <= safe_c && safe_c <= 0x0FB7) || 0x0FB9 == safe_c || 
             (0x20D0 <= safe_c && safe_c <= 0x20DC) || 0x20E1 == safe_c || 
             (0x302A <= safe_c && safe_c <= 0x302F) || 0x3099 == safe_c || 0x309A == safe_c
             ))

         )
      {
         c++;
         name_start = false;
      }
      else
         break;
   }

   tag_length = (int16_t) (c - element->pos);
   if (length != NULL)
      *length = tag_length;

   element->pos = c;

   /* There must be either whitespace or '>', or else the element's tag had an unacceptable
    * character. Also except '=' for attributes. */
   safe_c = deref(element->doc, c);
   if (tag_length == 0 ||
       (safe_c != '>' && safe_c != '=' && safe_c != ' ' && safe_c != '\t' && safe_c != 0x0a &&
        safe_c != 0x0d))
   {
      if (safe_c == '\0')
         element->doc->error = ALI_ERROR_DATA_INCOMPLETE;
      else
         element->doc->error = ALI_ERROR_TAG_INVALID;
      /* element tags and attribute tags have the same definition. 
       * http://www.w3.org/TR/REC-xml#NT-Stag 
       * http://www.w3.org/TR/REC-xml#NT-Attribute */
   }

   /* DOLATER keep this? */
   skip_whitespace(element);
}


static void
skip_end_tag(
   ali_element_info * element)
{
   assert(element != NULL);

   /* It possible, for nested tags, for whitespace to be next. 
    * A better fix might be to position better after ^e%F */
   skip_whitespace(element);

   /* Skip end tag http://www.w3.org/TR/REC-xml#NT-ETag */
   if (deref(element->doc, element->pos) == '<' && deref(element->doc, element->pos + 1) == '/')
   {
      element->pos += 2;        /* skip "</" */
      skip_element_tag(element, NULL);
      element->pos++;           /* skip '>' */
   }
   else if (deref(element->doc, element->pos) == '/' && deref(element->doc, element->pos + 1) == '>')
   {
      element->pos += 2;        /* skip "/>" */
   }
}

/* This differs from read_all_element_tags by not remembering what it's seen. It simply skips all.
 * This reduces the memory usage at the cost of performance to reprocess later when the data is
 * wanted. 
 * - This does not support CDATA yet. 
 * - This does not support processing instructions yet. */

static int
skip_content(
   ali_element_info * element,
   const char *name,
   int32_t length)
{
   ali_char c;
   uint8_t safe_c;
   int open_start_element_tag = 0;
   int open_end_element_tag = 0;
   int found_content;
   bool can_be_empty_element = true;


   assert(element != NULL);

   c = element->pos;
   safe_c = deref(element->doc, c);
   while (safe_c != '\0')
   {
      if (safe_c == '<')
      {
         can_be_empty_element = false;

         /* http://www.w3.org/TR/REC-xml#NT-ETag */
         if (deref(element->doc, c + 1) == '/' && strncmp(name, c + 2, length) == 0)
         {
            open_end_element_tag++;

            /* If we've closed all elements and we've seen at least one element, then we've skipped 
             * one element.
             * 
             * Only can be done when starting an end tag. 
             * This used to be done for every char, which besides being 
             * inefficient, happens to be true when an end tag is closed. */

            /* Don't count the last open end since it hasn't been closed yet. */
            if (open_end_element_tag > open_start_element_tag)
            {

               break;
            }

            c += 1 + length;
         }
         /* http://www.w3.org/TR/REC-xml#NT-Comment 
          * since comments may contain matching start or end tags, skip all contents */
         else if (deref(element->doc, c + 1) == '!' && deref(element->doc, c + 2) == '-' &&
            deref(element->doc, c + 3) == '-')
         {
            c += 4 - 1;

            do
            {
               c++;
               safe_c = deref(element->doc, c);
            }
            while (!(safe_c == '-' && *(c + 1) == '-' && *(c + 2) == '>') && safe_c != '\0');

            if (safe_c != '\0')
               c += 3 - 1;  /* move to the '>' in "-->".  It will be skipped shortly. */
            else
               break;

         }
         /* http://www.w3.org/TR/REC-xml#NT-STag */
         else if (strncmp(name, c + 1, length) == 0)
         {
            c += length;

            do
            {
               c++;
               safe_c = deref(element->doc, c);
            }
            while (safe_c != '>');

            /* handle empty elements by not counting them
             * http://www.w3.org/TR/REC-xml#NT-EmptyElemTag */
            if (*(c - 1) != '/')
               open_start_element_tag++;

         }
      }
      else if (can_be_empty_element && safe_c == '/' && deref(element->doc, c + 1) == '>')
      {
         /* stop where the content (which does not exist) ends 
            (which is before the close of the tag) */
         break;
      }


      c++;
      safe_c = deref(element->doc, c);
   }

   found_content = c != element->pos;
   element->pos = c;
   return found_content;
}


/* - This should be changed to read one element, to allow reading an element and then checking if
 * it's wanted, repeating until found or no more elements. 
 */

/* This function expects element->pos to be outside of the elements to find.  When it finds an
 * unknown closing tag or end of input, then it assumes it found all elements. */

/* -read element start tag
 * 
 * -read attribute 
 * -close tag 
 * -skip content, elements, comments, CDATA, processing instructions, close tag
 * 
 * These are the possible positions to read from */
static int
read_one_markup(
   ali_element_info * element)
{
   char terminator;
   uint8_t safe_c;
   ali_char starting_pos = element->pos;


   assert(element != NULL);

   /*  only elements and the root contain markup */
   if (element->element_type != tag_element && element->element_type != tag_none)
   {
      return false;
   }

   /* If position is past the element's name, then markup has already been looked for, and found. */
   /* This code should be called finish_content(), becaues it finishes the content partially 
    * read for identification.  It advances to the end of the content production.
    * http://www.w3.org/TR/REC-xml/#NT-content */
   if (element->type_of_last_markup_read != tag_none)
   {
      /* advance past the name. */
      element->pos = element->last_markup_name;

      element->pos += element->last_markup_name_length;


      /* http://www.w3.org/TR/REC-xml#NT-STag
       * 
       * Start reading after the name in a start tag.  This is before attributes, and before a
       * determination of a start tag versus an empty element is made. 
       * POSITION_POST_STAG_NAME */

      /* move from POSITION_POST_STAG_NAME to POSITION_CONTENT */
      skip_whitespace(element);

      if (element->type_of_last_markup_read == tag_attribute)
      {
         /* the tag is the attribute name.  Skip to the end of the attribute.
          * http://www.w3.org/TR/REC-xml#NT-Attribute */
         skip_whitespace(element);
         element->pos += 1;     /* Skip '=' */
         skip_whitespace(element);

         terminator = deref(element->doc, element->pos++);

         safe_c = deref(element->doc, element->pos);
         while (safe_c != terminator && safe_c != '\0')
         {
            element->pos++;
            safe_c = deref(element->doc, element->pos);
         }

         element->pos++;        /* skip terminator */
      }
      else if (element->type_of_last_markup_read == tag_element)
      {
         ali_char c;

         c = element->pos;

         /* skip attributes in the start tag
          * 
          * http://www.w3.org/TR/REC-xml#NT-STag 
          * http://www.w3.org/TR/REC-xml#NT-EmptyElemTag */
         safe_c = deref(element->doc, c);
         while (safe_c != '\0' && safe_c != '>')
         {
            c++;
            safe_c = deref(element->doc, c);
         }

         if (safe_c == '>' && *(c - 1) == '/')
         {
            /* http://www.w3.org/TR/REC-xml#NT-EmptyElemTag */
            c++;
            safe_c = deref(element->doc, c);
         }
         else
         {
            /* http://www.w3.org/TR/REC-xml#NT-content
             * 
             * skip content (which includes other markup) This is a serious code issue here.
             * Because this needs to skip an element, it essentially needs to incorporate most of
             * the XML spec */
            if (safe_c == '>')
               c++;
            element->pos = c;
            skip_content(element, element->last_markup_name, element->last_markup_name_length);


            /* Skip end tag */
            skip_end_tag(element);
            c = element->pos;
         }
         element->pos = c;
      }
      else if (element->type_of_last_markup_read == tag_comment)
         skip_to_end_of_comment(element);

      else if (element->type_of_last_markup_read == tag_instruction)
         skip_to_end_of_processing_instruction(element);
   }



/* http://www.w3.org/TR/REC-xml#NT-content
 Reading when any markup can be expected.  Attributes are expected if
 !element->start_tag_closed.  element->start_tag_closed is set true
 when a start tag is started or closed.
 POSITION_CONTENT */

 /* This should probably be it's own function. */

   /* Find the next markup */
   do
   {
      /* This is to skip whitespace before and after the root element. */
      skip_whitespace(element);

      /* Stop if leaving this element.  It is important that pos not change so that if this routine 
       * is called again then it gives the same result. */
      safe_c = deref(element->doc, element->pos);
      if (safe_c == '<' && deref(element->doc, element->pos + 1) == '/')
      {
         /* The caller tried to get more markup but there isn't any.  
          * Backup to the starting position so that the content for this 
          * element can be read. */
         element->pos = starting_pos;
         return false;
      }

      if (safe_c == '<' && deref(element->doc, element->pos + 1) == '!')
      {
         if (skip_dtd(element))
         {
            continue;
         }
      }


      /* Handle element */
      safe_c = deref(element->doc, element->pos);
      if (safe_c == '<')
      {
         element->start_tag_closed = true;

         element->pos++;
         safe_c = deref(element->doc, element->pos);

         if (safe_c == '!' && element->pos[1] == '-' && element->pos[2] == '-')
         {
            /* Read comment.  No text is kept for the markup name. */
            element->markup_name[element->count] = element->pos - 1;
            element->markup_type[element->count] = tag_comment;
            element->last_markup_name = element->pos - 1;
            element->markup_length[element->count] = 0;
            element->last_markup_name_length = element->markup_length[element->count];
            element->type_of_last_markup_read = tag_comment;
            element->count++;
         }
         else if (safe_c == '?')
         {
            /* Read the processing instruction target.  The markup_name
			    * is set to the PITarget.
             * http://www.w3.org/TR/REC-xml#NT-PITarget */
            element->pos++;
            element->markup_name[element->count] = element->pos;
            element->markup_type[element->count] = tag_instruction;
            element->last_markup_name = element->pos;
            skip_element_tag(element, &element->markup_length[element->count]);
            element->last_markup_name_length = element->markup_length[element->count];
            element->type_of_last_markup_read = tag_instruction;
            element->count++;
         }
         else
         {
            /* Read start tag. */
            element->markup_name[element->count] = element->pos;
            element->markup_type[element->count] = tag_element;
            element->last_markup_name = element->pos;
            skip_element_tag(element, &element->markup_length[element->count]);
            element->last_markup_name_length = element->markup_length[element->count];
            element->type_of_last_markup_read = tag_element;
            element->count++;
         }
         break;

      }
      else if (safe_c == '>')
      {
         /* DOLATER what if already closed? */
         element->start_tag_closed = true;
         element->pos++;
         starting_pos = element->pos;
      }
      else if (safe_c == '/' && deref(element->doc, element->pos + 1) == '>')
      {
         return false;  /* there is nothing more to read in this element */
      }
      else
      {
         if (!element->start_tag_closed)
         {
            /* skip past the element's start tag name. */
            if (element->pos == element->name)
               element->pos += element->length;

            /* if at the end of a prior attribute, move past the end */
            safe_c = deref(element->doc, element->pos);
            if (safe_c == '\'' || safe_c == '\"')
               element->pos++;

            /* read attribute */
            skip_whitespace(element);


            safe_c = deref(element->doc, element->pos);
            if (safe_c != '>')
            {
            /* Read attribute tag. */
            element->markup_name[element->count] = element->pos;
            element->markup_type[element->count] = tag_attribute;
            element->last_markup_name = element->pos;
            skip_element_tag(element, &element->markup_length[element->count]);
            element->last_markup_name_length = element->markup_length[element->count];
            element->type_of_last_markup_read = tag_attribute;
            element->count++;

            break;
            }
         }
         else
         {
            /* skip char.  It must be other content like character data. */
            element->pos++;
         }
      }
   }
   while (element->pos < element->doc->last_valid_char);


   return element->pos < element->doc->last_valid_char && deref(element->doc, element->pos) != '<';
}


#if 0
/* This follows the model of reading all markup, and is replaced by the model of reading markup
 * just in time, reducing overhead. */
static int
read_all_element_tags(
   ali_element_info * element)
{
   char terminator;


   assert(element != NULL);

   /* Read all elements */
   do
   {

      /* Stop if leaving this element. */
      if (*element->pos == '<' && *(element->pos + 1) == '/')
         break;

      /* DOLATER - make nicer */
      if (skip_comment(element))
      {
         continue;
      }

      /* Handle element */
      if (*element->pos == '<')
      {
         const char *c;

         element->pos++;

         element->start_tag_closed = true;

         /* Read start tag. */
         element->markup_name[element->count] = element->pos;
         element->markup_type[element->count] = tag_element;
         skip_element_tag(element, &element->markup_length[element->count]);
         element->count++;

         skip_whitespace(element);
         c = element->pos;


         if (*c == '/' && *(c + 1) == '>')
         {
            /* http://www.w3.org/TR/REC-xml#NT-EmptyElemTag */
            c += 2;             /* skip '/>' */
            element->pos = c;
         }
         else
         {
            /* Skip attributes, close start tags, and content. */
            while (*c != '\0')
            {
               if (*c == '<')
               {
                  if (*(c + 1) == '/')
                     break;
                  else
                  {
                     element->pos = c;
                     while (skip_whitespace(element) || skip_comment(element))
                     {
                     }
                     c = element->pos;
                  }
               }
               else
                  c++;
            }

            /* Skip end tag
             * 
             * http://www.w3.org/TR/REC-xml#NT-ETag */
            if (*c == '<' && *(c + 1) == '/')
            {
               c += 2;          /* skip "</" */
               element->pos = c;
               skip_element_tag(element, NULL);
               element->pos++;  /* skip '>' */
               c = element->pos;
            }
         }
         element->pos = c;
      }
      else if (*element->pos == '>')
      {
         element->start_tag_closed = true;
         element->pos++;
      }
      else
      {
         /* skip char.  It must be other content like character data. */
         element->pos++;

         if (!element->start_tag_closed)
         {
            /* read attribute */

            /* Read attribute tag. */
            element->markup_name[element->count] = element->pos;
            element->markup_type[element->count] = tag_attribute;
            element->count++;

            /* http://www.w3.org/TR/REC-xml#NT-Attribute */
            skip_element_tag(element, &element->markup_length[element->count - 1]);
            element->pos += 1;  /* Skip '=' */
            skip_whitespace(element);

            terminator = *element->pos++;

            while (*element->pos != terminator && *element->pos != '\0')
               element->pos++;

            element->pos++;     /* skip terminator */
         }
      }
   }
   while (*element->pos != '\0');

   return 0;
}
#endif


/* Decode the string from start to end, appending the results to string, or allocating a new string 
 * if needed.
 */

static char *
decode_string(
   ali_doc_info * doc,          /* incase needed to track malloc */
   char *string,
   int decode,   /* true means decode, false means don't touch the chars (comment/cdata/pi) */
   const char *start,
   const char *end,
   char * dest, /* buffer where to store the content.  NULL means to allocate and return. */
   uint32_t dest_size /* size of dest. */
   )
{
   uint32_t size;
   uint32_t oldSize;
   char *result;
   char *c;


   assert(start != NULL);
   assert(end != NULL);

   if (start > end)
      return NULL;

   size = end - start + 1 + 1;

   if (string == NULL)
   {
      if (dest != NULL)
      {
          result = dest;
      }
      else
      {
          result = (char *) malloc(sizeof(*result) * size);
          if (result == NULL)
          {
             doc->error = ALI_ERROR_MEMORY_FAILURE;
             longjmp(doc->environment, ALI_ERROR_MEMORY_FAILURE);
          }
      }
      c = result;
   }
   else
   {
      /* This can be an over estimate.  Entities can use less space. */
      oldSize = strlen(string);
      size += oldSize;
      result = (char *) realloc(string, sizeof(*result) * size);
      if (result == NULL)
      {
         doc->error = ALI_ERROR_MEMORY_FAILURE;
         longjmp(doc->environment, ALI_ERROR_MEMORY_FAILURE);
      }
      c = &result[oldSize];
   }


   if (dest_size == 0)
       dest_size = 0xffffffff;

   if (decode)
   {
      while (start <= end && dest_size > 0)
      {
         /* http://www.w3.org/TR/REC-xml#sec-predefined-ent
          * 
          * If non predefined entities are ever supported, it would probably be better to move these
          * into that entity list instead of special casing them like this. */
         if (*start == '&')
         {
            if (strncmp(start, "&lt", 3) == 0)
            {
               *c++ = '<';
               dest_size--;
               start += 3;
            }
            else if (strncmp(start, "&gt", 3) == 0)
            {
               *c++ = '>';
               dest_size--;
               start += 3;
            }
            else if (strncmp(start, "&amp", 4) == 0)
            {
               *c++ = '&';
               dest_size--;
               start += 4;
            }
            else if (strncmp(start, "&apos", 5) == 0)
            {
               *c++ = '\'';
               dest_size--;
               start += 5;
            }
            else if (strncmp(start, "&quot", 5) == 0)
            {
               *c++ = '"';
               dest_size--;
               start += 5;
            }
            else if (*(start + 1) == '#')
            {
               long unsigned int v;
               
               /* http://www.w3.org/TR/REC-xml#dt-charref */
               /* Characters may be one, two or four byte values */
               start += 2;
               if (*start == 'x')
               {
                  start += 1;
                  sscanf_s(start, "%lx", &v);
               }
               else
               {
                  sscanf_s(start, "%ld", (long int *)&v);
               }
               
               /* a utf32to8 converter. */
               if (v <= 0x007F)
               {
                  *c++ = (unsigned char)v;
                  dest_size--;
               }
               else if (v <= 0x07FF) {
                  *c++ = (unsigned char)((v >> 6) + 0xC0);
                  *c++ = (unsigned char)((v & 0x3F) | 0x80);
                  dest_size -= 2;
               }
               else if (v <= 0xFFFF) {
                  *c++ = (unsigned char)((v >> 12) + 0xE0);
                  *c++ = (unsigned char)(((v >> 6) & 0x3F) | 0x80);
                  *c++ = (unsigned char)((v & 0x3F) | 0x80);
                  dest_size -= 3;
               }
               else 
               {
                  *c++ = (unsigned char)((v >> 18) + 0xF0);
                  *c++ = (unsigned char)(((v >> 12) & 0x3F) | 0x80);
                  *c++ = (unsigned char)(((v >> 6) & 0x3F) | 0x80);
                  *c++ = (unsigned char)((v & 0x3F) | 0x80);
                  dest_size -= 4;
               }
            }
            
            /* skip the trailing ';'. */
            while ((('0' <= *start && *start <= '9') ||
                  ('a' <= *start && *start <= 'f') ||
                  ('A' <= *start && *start <= 'F')) &&
               start < end)
            {
               start++;
            }
            if (*start == '\0')
            {
               doc->error = ALI_ERROR_DATA_INCOMPLETE;
               longjmp(doc->environment, ALI_ERROR_DATA_INCOMPLETE);
            }
            else if (*start != ';')
            {
               doc->error = ALI_ERROR_TAG_INVALID;
               longjmp(doc->environment, ALI_ERROR_TAG_INVALID);
            }
            start++;
         }
         /* http://www.w3.org/TR/REC-xml#sec-line-ends */
         else if (*start == 0x0d)
         {
            *c++ = '\n';
            dest_size--;
            start++;
         }
         /* http://www.w3.org/TR/REC-xml#sec-line-ends */
         else if (*start == 0x0a)
         {
            /* The 0x0a0d combo should be read as one.  If there is a 0x0d, then let that add the
             * '\n'. */
            if (*(start - 1) != 0x0d)
            {
               *c++ = '\n';
               dest_size--;
            }
            start++;
         }
         else if (*start == '\t')
         {
            *c++ = ' ';
            dest_size--;
            start++;
         }
         else if (*start == '<' && strncmp(start, "<![CDATA", 8) == 0)
         {
            start += 8;

            while (start <= end && dest_size > 0 && 
               *start != ']' && strncmp(start, "]]>", 3) != 0)
            {
               *c++ = *start++;
               dest_size--;
            }

            start += 3;
         }
         else if (!IS_UTF_8_SINGLE(*start) &&
            (doc->options & ALI_OPTION_EXP_CONVERT_UTF8_TO_ISO_8859_1))
         {
            /* This assumes ISO-8859-1, but the encoding could be checked for
             * for other variations and then treat them appropriately. */
            if (*(unsigned char *) start == 0xC2)
            {
               start++;
               *c++ = *start++;
               dest_size--;
            }
            else if (*(unsigned char *) start == 0xC3)
            {
               start++;
               *c++ = (char) (*start++ + 64);
               dest_size--;
            }
            else
            {
               /* What to do?  The character cannot be represented.  
                * The code could output character references (&#x0a1b)
                * which has the advantage that the content could be 
                * edited and then restored on a UTF-8 system. */
               start++;
               *c++ = '?';
            }
            
         }
         else
         {
            *c++ = *start++;
            dest_size--;
         }
      }
   }
   else
   {
      while (start <= end && dest_size > 0)
      {
         *c++ = *start++;
         dest_size--;
      }
   }
   *c++ = '\0';
   dest_size--;

   return result;
}


/* Return a string for the content.  The content is decoded.  The string is allocated and must be
 * ali_free. 
 * element->pos must be set.
 * 
 * NOTE This routine should probably not return all content combined 
 * together.  This removes information about where markup is.  For 
 * instance, if a word is bold in a sentence, where is that word? 
 * In other words, the algorithm is fine if the XML is data structures 
 * but not if it's XHTML. 
 *
 * \todo Allow reading of part of the content, and not all of the atribute
 * or element.  This would enable commands like "^e%d-%d-%d" for "7-4-1776".
 * There are two parts.  The first is stop reading once enough is read.
 * For instance, stop after a non digit is found for the above.  Second, 
 * reading the next element needs to resume correctly.  I think the code 
 * currently relies on the position being at the end of the content.
 */
static char *
get_content(
   ali_element_info * element,
   bool advance_to_content, /* true to advance reading to where the content starts
                             * for the markup_type.  False allows reading multiple values 
                             * from the same content. */
   const char *prefix, /* This string must be present at the start.  It is skipped. */
   const char *suffix, /* This string must be present at the end. */
   char * dest, /* buffer where to store the content.  NULL means to allocate and return. */
   uint32_t dest_size /* size of dest. */
   )
{
   char *result = NULL;
   char terminator = '\0';
   const char *attribute_start;
   const char *attribute_end;
   int suffix_length;
   uint8_t safe_c;
   int markup_type;
   uint32_t width_remaining = dest_size; /* limit the chars read to the width */


   assert(element != NULL);
   assert(prefix != NULL);
   assert(suffix != NULL);

   if (element->last_markup_read != ali_element_none)
      markup_type = element->markup_type[element->last_markup_read];
   else
      markup_type = element->element_type;


   if (advance_to_content)
   {
      /* advance to just after the markup_name's end. */
      if (element->last_markup_read != ali_element_none)
      {
         element->pos = &element->markup_name[element->last_markup_read]
         [element->markup_length[element->last_markup_read]];
      }
      else
      {
         element->pos = &element->name[element->length];
      }
   }

   if (markup_type == tag_attribute)
   {
      if (advance_to_content)
      {
         /* http://www.w3.org/TR/REC-xml#NT-AttValue */
         safe_c = deref(element->doc, element->pos);
         while (safe_c != '"' && safe_c != '\'' && safe_c != '\0')
         {
            element->pos++;
            safe_c = deref(element->doc, element->pos);
         }
         element->pos++;

         terminator = safe_c;
      }
      else
      {
         /* need to recover the terminator.  It's after the markup_name. */
         ali_char temp = element->pos;
         element->pos = &element->markup_name[element->last_markup_read]
                         [element->markup_length[element->last_markup_read]];
         skip_whitespace(element);
         element->pos++;
         skip_whitespace(element);
         safe_c = deref(element->doc, element->pos);
         terminator = safe_c;

         element->pos = temp;
      }
   }
   else if (markup_type == tag_element)
   {
      /* DOLATER - This is hacked to get the content in an element without other elements nested.
       * It needs to skip nested elements, concating all content together.  I assume this will get
       * done when some input requires it... */

      if (advance_to_content)
      {
         /* http://www.w3.org/TR/REC-xml#NT-AttValue */
         safe_c = deref(element->doc, element->pos);
         while (safe_c != '>' && safe_c != '\0')
         {
            element->pos++;
            safe_c = deref(element->doc, element->pos);
         }
         element->start_tag_closed = true;

         if (safe_c == '>' && *(element->pos - 1) == '/')
         {
            /* http://www.w3.org/TR/REC-xml#NT-EmptyElemTag */
            return NULL;
         }

         element->pos++;
      }

      terminator = '<';
   }
   else if (markup_type == tag_comment)
   {
      /* http://www.w3.org/TR/REC-xml#NT-Comment */
      if (advance_to_content)
         element->pos += 4;
      terminator = '-';
   }
   else if (markup_type == tag_instruction)
   {
      /* http://www.w3.org/TR/REC-xml#NT-PI */
      if (advance_to_content)
         element->pos++;
      terminator = '?';
   }
   else
   {
      /* Unknown markup_type */
      assert(false);
   }


   if (terminator == '\0')
      return NULL;              /* DOLATER error */

   while (*prefix != '\0')
   {
      if (*prefix == deref(element->doc, element->pos))
      {
         element->pos++;
         prefix++;
      }
      else if (*prefix == ' ' || *prefix == '\t' || *prefix == 0x0a || *prefix == 0x0d)
      {
         skip_whitespace(element);
         prefix++;
      }
      else
      {
         return NULL;           /* prefix mismatch, and it's required for a match */
      }
   }

   attribute_start = element->pos;

   suffix_length = strlen(suffix);
   if (width_remaining == 0)
      width_remaining = 0xffffffff;
   safe_c = deref(element->doc, element->pos);
   while (width_remaining-- > 0 &&
      (safe_c != terminator ||
         (terminator == '<' && strncmp(element->pos, "<![CDATA", 8) == 0) ||
         (terminator == '-' && element->pos[1] != '-' && element->pos[2] != '>') ||
         (terminator == '?' && element->pos[1] != '>')))
   {
      if (safe_c == *suffix)
      {
         /* DOLATER whitespace isn't checked correctly.  Any whitespace should match any
          * whitespace, so strncmp can't really be used. */
         if (strncmp(element->pos, suffix, suffix_length) == 0)
            break;
      }

      /* handle predefined entities.  Each one should count for one char 
       * in terms of width counting. */
      if (safe_c == '&' && markup_type == tag_element)
      {
         while (safe_c != ';')
            safe_c = deref(element->doc, ++element->pos);
      }

      /* advance through CDATA zones.
       * DOLATER: notice that we don't respect width or the suffix. 
       * Resuming such partial reads means keeping more state. */
      if (safe_c == '<' && strncmp(element->pos, "<![CDATA", 8) == 0 &&
         markup_type == tag_element)
      {
         width_remaining++;
         element->pos += 8;
         safe_c = deref(element->doc, element->pos);
         while (safe_c != '\0' && safe_c != ']' && strncmp(element->pos, "]]>", 3) != 0)
         {
            element->pos++;
            safe_c = deref(element->doc, element->pos);
            width_remaining--;
         }

         if (safe_c == ']')
         {
            element->pos += 3;
            safe_c = deref(element->doc, element->pos);
         }
      }
      else
      {
         element->pos++;
         safe_c = deref(element->doc, element->pos);
      }
   }

   attribute_end = element->pos - 1;

   /* Check that the suffix was obeyed.  If the terminator was reached but there was a suffix, then 
    * the suffix wasn't found. */
   if (*suffix != '\0' && *attribute_end == terminator)
      return NULL;

   result = decode_string(element->doc, NULL, 
      markup_type == tag_element || markup_type == tag_attribute,
      attribute_start, attribute_end, dest, dest_size);

   return result;
}


/* Get the value for a name in the XML declaration. */
static ali_char
get_next_xml_declaration(
   ali_doc_info * doc,
   ali_element_info * element,
   const char * name)
{
   int name_length;
   ali_char value = NULL;
   char terminator;


   skip_whitespace(element);
   
   name_length = strlen(name);
   if (strncmp(name, element->pos, name_length) == 0)
   {
       element->pos += name_length;

       if (deref(element->doc, element->pos) != '=')
       {
           doc->error = ALI_ERROR_XML_DECLARATION_INVALID;
           return NULL;
       }
       element->pos++;

       terminator = deref(element->doc, element->pos);
       if (terminator != '\'' && terminator != '"')
       {
           doc->error = ALI_ERROR_XML_DECLARATION_INVALID;
           return NULL;
       }
       element->pos++;
       
       value = element->pos;
       
       do
       {
           element->pos++;
       }
       while (deref(element->doc, element->pos) != terminator);
       
       element->pos++;
   }
   
   return value;
}

/*! \brief Parse an XML declaration. 
 *
 * \return Return ALI_ERROR_NONE if a declaration was found and was well formed. */
static int
parse_xml_declaration(
   ali_doc_info * doc,
   ali_element_info * element)
{
   ali_char version;
   ali_char encoding;
   ali_char standalone;
   

   assert(element != NULL);


   /* http://www.w3.org/TR/REC-xml#NT-prolog 
    * http://www.w3.org/TR/REC-xml/#NT-XML_Decl
    * Do encodings we handle first.
    */
   if (strncmp("<?xml", element->pos, 5) == 0)
   {
       element->pos += 5;

       version = get_next_xml_declaration(doc, element, "version");
       if (version == NULL)
       {
           doc->error = ALI_ERROR_XML_DECLARATION_INVALID;
       }
       else
       {
           encoding = get_next_xml_declaration(doc, element, "encoding");

           /* Compare the encoding to those we know about to identify them. 
            * This overrides earlier encoding gueses. */
           if (encoding == NULL || _strnicmp(encoding, "UTF-8", 5) == 0)
           {
               doc->encoding = encoding_UTF_8;
           }
           else if (_strnicmp(encoding, "ISO-8859-", 9) == 0)
           {
               doc->encoding = encoding_ISO_8859_N;
           }
           else if (_strnicmp(encoding, "US-ASCII", 8) == 0)
           {
               doc->encoding = encoding_US_ASCII;
           }
           

           /* http://www.w3.org/TR/REC-xml/#NT-SDDecl */
           standalone = get_next_xml_declaration(doc, element, "standalone");
           if (standalone != NULL)
           {
               if (_strnicmp(standalone, "yes", 3) == 0)
               {
                   doc->standalone = true;
                   doc->standalone_declared = true;
                   printf("standalone=yes\n");
               }
               else if (_strnicmp(standalone, "no", 2) == 0)
               {
                   doc->standalone = false;
                   doc->standalone_declared = true;
                   printf("standalone=no\n");
               }
           }
           

           skip_whitespace(element);
           
           if (strncmp("?>", element->pos, 2) != 0)
           {
               doc->error = ALI_ERROR_XML_DECLARATION_INVALID;
           }
           else
           {
               element->pos += 2;
           }
       }
   }
   /* Spend time to identify the document based on how the
    * XML declaration is written compared to how it should
    * look in various encodings.  If none match, then it's
    * probably not a XML document. */
   else if (doc->size - (element->pos - doc->text) >= 4)
   {
       /* http://www.w3.org/TR/REC-xml/#sec-guessing */
       if (element->pos[0] == 0x3C &&
           element->pos[1] == 0x00 &&
           element->pos[2] == 0x3F &&
           element->pos[3] == 0x00)
       {
            doc->encoding = encoding_UTF_16;
       }
       else if (element->pos[0] == 0x00 &&
           element->pos[1] == 0x3C &&
           element->pos[2] == 0x00 &&
           element->pos[3] == 0x3F)
       {
            doc->encoding = encoding_UTF_16_SWAPPED;
       } 
       else if (element->pos[0] == 0x3C &&
           element->pos[1] == 0x00 &&
           element->pos[2] == 0x00 &&
           element->pos[3] == 0x00)
       {
            doc->encoding = encoding_UTF_32;
       }
       else if (element->pos[0] == 0x00 &&
           element->pos[1] == 0x00 &&
           element->pos[2] == 0x00 &&
           element->pos[3] == 0x3C)
       {
            doc->encoding = encoding_UTF_32_SWAPPED;
       }
       else if (element->pos[0] == 0x4C &&
           element->pos[1] == 0x6F &&
           (unsigned char) element->pos[2] == 0xA7 &&
           (unsigned char) element->pos[3] == 0x94)
       {
            doc->encoding = encoding_EBCDIC;
       }
       else
       {
           /* There is text in the document but it is lacking a XML declaration.
            * Therefore, treat it as not an XML document. */
           doc->error = ALI_ERROR_NOT_XML_DOCUMENT;
       }
   }
   else
   {
       /* The document is too little to even have a required XML declaration. */
       doc->error = ALI_ERROR_NOT_XML_DOCUMENT;
   }


   return doc->error;
}


static void
remove_markup(
   ali_element_info * current_element,
   int i)
{
   assert(current_element != NULL);
   assert(i >= 0);
   assert(i < ali_element_max);

   /* advance current_element->pos past the end of the markup */
   if (current_element->last_markup_read != ali_element_none)
   {
      if (current_element->type_of_last_markup_read == tag_element)
      {
         uint8_t safe_c;

         current_element->pos = current_element->last_markup_name;// + current_element->last_markup_length;
         safe_c = deref(current_element->doc, current_element->pos);
         if (safe_c == '>')
            current_element->pos++;
         skip_content(current_element, current_element->last_markup_name, current_element->last_markup_name_length);


         /* Skip end tag */
         skip_end_tag(current_element);
      }
      else if (current_element->type_of_last_markup_read == tag_attribute)
      {
         uint8_t safe_c;

         safe_c = deref(current_element->doc, current_element->pos);
         while (safe_c != '\0' &&
            safe_c != '\'' && safe_c != '\"')
         {
            current_element->pos++;
            safe_c = deref(current_element->doc, current_element->pos);
         }
         if (safe_c == '\'' || safe_c == '\"')
            current_element->pos++;

         /* since element->pos has been moved to after this attribute, it's 
            again before the close of the start tag. */
         if (current_element->start_tag_closed)
            current_element->start_tag_closed = false;
      }
      else if (current_element->type_of_last_markup_read == tag_comment)
         skip_to_end_of_comment(current_element);

      else if (current_element->type_of_last_markup_read == tag_instruction)
         skip_to_end_of_processing_instruction(current_element);
   }

   /* If we needed to keep the info around for use, we could alternatively mark the element as read 
    * and free all later.  This is slightly preferred for now for better performance.  Is this
    * optimizing too soon? Requirements unclear. */
   if (current_element->count > 1 && current_element->count < ali_element_max - 1)
   {
      memcpy((void *) &current_element->markup_name[i], &current_element->markup_name[i + 1],
         (current_element->count - (i + 1)) * sizeof(current_element->markup_name[i]));
      current_element->markup_name[current_element->count - 1] = NULL;  /* clarify for debugging */
      memcpy(&current_element->markup_length[i], &current_element->markup_length[i + 1],
         (current_element->count - (i + 1)) * sizeof(current_element->markup_length[i]));
      memcpy(&current_element->markup_type[i], &current_element->markup_type[i + 1],
         (current_element->count - (i + 1)) * sizeof(current_element->markup_type[i]));
   }
   current_element->count--;

   current_element->last_markup_read = ali_element_none;
   current_element->type_of_last_markup_read = tag_none;
}


static void
new_current_element(
   ali_doc_info * doc,
   ali_element_ref parent,
   const char *pos,
   int32_t line_number,
   ali_char name,
   int32_t name_length,
   int8_t element_type)
{
   ali_element_info *current_element;

   assert(doc != NULL);

   /* Make sure there is space for another element */
   if (doc->elements_count >= doc->elements_size)
   {
      /* Resize the elements to add one more */
      if (doc->elements_size == 0)
      {
         doc->elements_size = ALI_CONFIG_DEFAULT_NESTED_ELEMENT_COUNT;
         doc->elements = (ali_element_info *) malloc(doc->elements_size * sizeof(*(doc->elements)));
      }
      else
      {
         doc->elements_size = (uint16_t) (doc->elements_size * 2);
         doc->elements =
            (ali_element_info *) realloc(doc->elements,
            doc->elements_size * sizeof(*(doc->elements)));
      }
   }

   if (doc->elements == NULL)
   {
      doc->error = ALI_ERROR_MEMORY_FAILURE;
      longjmp(doc->environment, ALI_ERROR_MEMORY_FAILURE);
   }

   doc->elements_count++;


   /* Allocate a space for info about the current element. */
   current_element = &doc->elements[doc->elements_count - 1];
   doc->current_element = current_element;
   current_element->parent = parent;
   current_element->elements_read = false;
   current_element->data_used = false;
   current_element->data_unavailable = false;
   current_element->new_element = true;
   current_element->count = 0;
   current_element->type_of_last_markup_read = tag_none;
   current_element->last_markup_name = name;
   current_element->last_markup_name_length = name_length;
   current_element->pos = pos;
   current_element->line_number = line_number;
   current_element->name = name;
   current_element->length = name_length;
   current_element->element_type = element_type;
   current_element->last_markup_read = ali_element_none;
   current_element->element = doc->next_element;
   current_element->start_tag_closed = false;
   current_element->doc = doc;

   doc->next_element = (ali_element_ref) (doc->next_element + 1);
}



static ali_element_info *
find_element(
   const ali_doc_info * doc,
   ali_element_ref element
   )
{
   int16_t i;


   /* Search in reverse order.  Typically elements are LIFO. */
   for (i = (int16_t) (doc->elements_count - 1); i >= 0; i--)
   {
      if (doc->elements[i].element == element)
         return &doc->elements[i];
   }

   return NULL;
}

static void
delete_element(
   ali_doc_info * doc,
   ali_element_info ** element  /* the element to delete.  Set to the element's parent for use. */
   )
{
   uint16_t elements_to_move;
   uint32_t i;
   ali_element_info * parent;


   assert(doc != NULL);
   assert(element != NULL);

   i = doc->elements_count - 1;
   assert(*element == &doc->elements[i]);

   if ((*element)->element_type == tag_element)
   {
      /* read the rest of the markup and then exit the element.  This is needed
         to close up the markup and be at a good position. */
      skip_content(*element, (*element)->name, (*element)->length);
      skip_end_tag(*element);
   }

   /* if the parent element's pos isn't advanced as far as this, move it up. */
   parent = find_element(doc, (*element)->parent);
   if (parent->pos < (*element)->pos)
      parent->pos = (*element)->pos;

   if (i < doc->elements_count)
   {
      elements_to_move = (uint16_t) (doc->elements_count - i - 1);
      if (elements_to_move > 0)
         memcpy(&doc->elements[i], &doc->elements[i + 1],
            elements_to_move * sizeof(*doc->elements));

      doc->elements_count--;
   }

   /* Delete the element doc.  We could leave this information around in case it's needed and
    * delete all at the very end.  But removing elements yields better performance and lower
    * resources. */
   if (i == 0)
      *element = NULL;
   else
      *element = &doc->elements[i - 1];
}


/*! \brief Is this the first time an element has been read?
 * 
 * This is often used to know when to allocate, initialize, or construct 
 * information for an element that contains lots of data.
 * 
 * \return true if the element has been read once, false if read more than once.
 * 
 * \see ali_in */

/* This could have been an argument passed to every callback, but this information isn't always
 * used, and it seems like the user's code is smaller if they occasionally make an API call to get
 * it. */
bool
ali_is_element_new(
   const ali_doc_info * doc,    /*!< the document to input from */
   ali_element_ref element)     /*!< which XML element in the document to input from */
{
   ali_element_info *current_element;


   /* Check for err */
   assert(doc != NULL);

   /* Search for the element specified. */
   current_element = doc->current_element;
   if (current_element->element != element)
   {
      current_element = find_element(doc, element);
   }

   if (current_element == NULL)
   {
      /* DOLATER error if invalid element? (element != 0) make error robust */
      return ALI_ERROR_ELEMENT_INVALID;

   }

   return current_element->new_element;
}


/*! \brief Is this the final time an element will be read?
 * 
 * This is generally used to see if all the data is read so that a 
 * structure or object can be created or closed or finalized.
 * 
 * \return false if the element will be read more, true if not.
 * 
 * \see ali_is_element_new, ali_in */

/* This is needed to allow creation of objects/structures that need information for creation. */
bool
ali_is_element_done(
   const ali_doc_info * doc,    /*!< the document to input from */
   ali_element_ref element)     /*!< which XML element in the document to input from */
{
   ali_element_info *current_element;

   /* Check for err */
   assert(doc != NULL);
   assert(doc->current_element->element == element);  /* OK? */

   current_element = doc->current_element;

   /* Done if there's an error or there is no more markup. */
   if (doc->error != ALI_ERROR_NONE)
      return true;

   if (current_element->count == 0 && !current_element->elements_read)
      current_element->elements_read = !read_one_markup(current_element);

   /* If there is no more markup or it wasn't used by the callback */
   return current_element->count == 0 || !current_element->data_used;
}


static char *
get_markup_name(ali_char name, int32_t length, int8_t type)
{
   char * result = NULL;

   /* http://www.w3.org/TR/xpath20/#doc-xpath-SequenceType */
   if (type == tag_element || type == tag_none)
   {
      /* "/name" */
      result = (char *) malloc(1 + length + 1);
      strcpy_s(result, 2, "/");
      strncat_s(result, 1 + length + 1, name, length);
      result[1 + length] = '\0';
   }
   else if (type == tag_attribute)
   {
      /* "[@name]" */
      result = (char *) malloc(2 + length + 1 + 1);
      strcpy_s(result, 3, "[@");
      strncat_s(result, 2 + length + 1 + 1, name, length);
      strcpy_s(&result[2 + length], 2, "]");
   }
   else if (type == tag_comment)
   {
      /* "/comment()" */
      result = (char *) malloc(10 + 1);
      strcpy_s(result, 11, "/comment()");
   }
   else if (type == tag_instruction)
   {
      /* "/processing-instruction(name)" */
      result = (char *) malloc(24 + length + 1 + 1);
      strcpy_s(result, 25, "/processing-instruction(");
      strncat_s(result, 24 + length + 1 + 1, name, length);
      strcpy_s(&result[24 + length], 2, ")");
   }

   return result;
}

/* This combines the element's name with the last markup read inside it. */
static char *
get_element_name(ali_element_info *current_element, ali_element_ref path_end)
{
   char * last_name = NULL;
   char * name = NULL;

   /* only the last element should list the last markup read.  Otherwise
    * every element will appear twice in the path, once for itself, and 
    * once by it's parent. */
   if (current_element->element == path_end &&
      current_element->last_markup_read != ali_element_none)
      last_name = get_markup_name(current_element->last_markup_name, 
         current_element->last_markup_name_length, 
         current_element->type_of_last_markup_read);

   name = get_markup_name(current_element->name, 
      current_element->length, 
      current_element->element_type);

   /* name = name + last_name */
   if (name != NULL && last_name != NULL)
   {
	  int tmp = strlen(name) + 1 + strlen(last_name) + 1;
      name = (char *) realloc(name, strlen(name) + 1 + strlen(last_name) + 1);
      if (name != NULL)
      {
         strcat_s(name, tmp, last_name);
      }
   }

   if (last_name != NULL)
      free(last_name);

   return name;
}

static void
build_element_path(
   const ali_doc_info * doc,    /*!< the document to input from */
   ali_element_ref element,     /*!< which XML element in the document to input from */
   ali_element_ref path_end,
   char ** buffer)
{
   ali_element_info *current_element = find_element(doc, element);

   if (current_element != NULL)
   {
      if (current_element->parent == 2 || current_element->parent == 0)
      {
         /* the root or first element, so start the path */
         *buffer = get_element_name(current_element, path_end);
      }
      else
      {
         char * next_name;
         build_element_path(doc, current_element->parent, path_end, buffer);
         next_name = get_element_name(current_element, path_end);

         /* *buffer = *buffer + next_name */
         if (*buffer != NULL && next_name != NULL)
         {
			int tmp = strlen(*buffer) + 1 + strlen(next_name) + 1;
            *buffer = (char *) realloc(*buffer, strlen(*buffer) + 1 + strlen(next_name) + 1);
            if (*buffer != NULL)
            {
               strcat_s(*buffer, tmp, next_name);
            }
         }

         if (next_name != NULL)
            free(next_name);
      }

   }
}


/* \brief Get the XPath to the element
 * 
 * Used when reading all markup ("^*") to see what was read.
 * 
 * \return string containg the path.  Free when done.
 * 
 * \see ali_in */

/* This is needed to allow creation of objects/structures that need information for creation. */
static char *
get_element_path(
   const ali_doc_info * doc,    /*!< the document to input from */
   ali_element_ref element)     /*!< which XML element in the document to input from */
{
   char * result = NULL;
   ali_element_info *current_element;

   /* Check for err */
   assert(doc != NULL);

   current_element = doc->current_element;

   if (current_element->element != element)
      current_element = find_element(doc, element);

   if (current_element == NULL)
      return NULL;

   build_element_path(doc, element, element, &result);


   return result;
}


/* This checks for Unicode byte order marks to identify a Unicode encoding
 * and an endian difference.  The encoding is best guess and is overridden
 * by an explicit encoding in the declaration. */
static void check_byte_order_mark(
    ali_doc_info * doc,    /*!< the document to input from */
    int32_t *pos)
{
    /* Check for a Byte Order Mark.
     * http://www.w3.org/TR/REC-xml/#charencoding */
    if (doc->size >= 4)
    {
        uint32_t byte_order_mark;
        
        byte_order_mark = *(uint32_t *) doc->text;
        /* 0x00 0x00 0xFE 0xFF */
        if (byte_order_mark == 0xfeff)
        {
            *pos += sizeof(byte_order_mark);
            doc->endian = endian_native;
            doc->encoding = encoding_UTF_32;
        }
        /* 0xFF 0xFE 0x00 0x00 */
        else if (byte_order_mark == (uint32_t) 0xfffe << 16)
        {
            *pos += sizeof(byte_order_mark);
            doc->endian = endian_swapped;
            doc->encoding = encoding_UTF_32_SWAPPED;
        }
    }
    if (doc->encoding == encoding_unknown && doc->size >= 2)
    {
        uint16_t byte_order_mark;
        
        byte_order_mark = *(uint16_t *) doc->text;
        /* 0xFE 0xFF */
        if (byte_order_mark == 0xfeff)
        {
            *pos += sizeof(byte_order_mark);
            doc->endian = endian_native;
            doc->encoding = encoding_UTF_16;
        }
        /* 0xFF 0xFE */
        else if (byte_order_mark == 0xfffe)
        {
            *pos += sizeof(byte_order_mark);
            doc->endian = endian_swapped;
            doc->encoding = encoding_UTF_16_SWAPPED;
        }
    }
    if (doc->encoding == encoding_unknown && doc->size >= 3)
    {
        /* 0xEF 0xBB 0xBF */
        if ((uint8_t) doc->text[0] == 0xef &&
            (uint8_t) doc->text[1] == 0xbb &&
            (uint8_t) doc->text[2] == 0xbf)
        {
            *pos += 3;
            doc->encoding = encoding_UTF_8;
        }
    }
}
               
               
/*! \brief Open an Ali document for input.
 * 
 * Start input from the file named and return an ali_element_ref needed by all 
 * other Ali functions.  Optionally parse a declaration.
 * 
 * \return A non zero value is a valid ali_element_ref which means the XML document
 * was successfully opened for reading.  Use the ali_element_ref to read from the XML document
 * by passing it to ali_in.  A zero indicates an error opening the document.  Check 
 * stdio's errno to determine the error's cause.
 *
 * \see ali_in, ali_close */

ali_element_ref
ali_open(
   ali_doc_info ** new_doc,     /*!< the document to input from */
   const char *file_name,       /*!< the name of the XML file that the document 
                                 * inputs data from.  The file may be encoded using
                                 * UTF-8, ISO-8859-1 or US-ASCII.  The encodings UTF-16
                                 * and UTF-32 are rejected.  Others will be tried and 
                                 * should work if they are like C strings (nul terminated). */
   uint32_t options,            /*!< options for inputing, like expecting xml declarations */
   void *data                   /*! data passed to callbacks. Usually a pointer to an app
                                 * structure used to store the document's data. Set 
                                 * to NULL if your app does not need it. */
   )
{
   ali_element_ref result = 0;
   ali_doc_info * doc;

   /* option replaces a Boolean which is sometimes true.  Don't allow that value for a while
    * until it's probably no longer used. */
   assert(options != 1);

   /* Check for err */
   assert(file_name != NULL);
   assert(new_doc != NULL);
   doc = *new_doc = (ali_doc_info *) malloc(sizeof(*doc));
   if (doc != NULL)
   {
      /* needed when the xml declaration is parsed. */
      if (!setjmp(doc->environment))
      {

         doc->error = ALI_ERROR_NONE;

         doc->data = data;

         doc->size = 0;
         doc->text = NULL;

         doc->elements = NULL;
         doc->elements_size = 0;
         doc->elements_count = 0;

         doc->options = options;


         fopen_s(&doc->file_in, file_name, "r");

         if (doc->file_in)
         {
            fseek(doc->file_in, 0, SEEK_END);
            doc->size = ftell(doc->file_in);
            fseek(doc->file_in, 0, SEEK_SET);

            if (doc->size > 0)
            {
               /* Extra space for a null terminating character is included. 
                * This design enables safe usage of the standard str* functions, 
                * but stops memory mapping the file. */
               doc->text = (ali_char) malloc(doc->size + 1);
            }

            if (doc->text != NULL)
            {
               int32_t pos = 0;

               /* For some reason, fread returns a smaller amount than the filesize. Fix this so null
                * termination works. */
               int32_t l = fread((void *) doc->text, sizeof(*doc->text), doc->size, doc->file_in);

               doc->size = l;
               doc->last_valid_char = &doc->text[doc->size];
               *(char *) doc->last_valid_char = '\0';
               doc->current_element = NULL;

               doc->endian = endian_unknown;
               doc->encoding = encoding_unknown;

               check_byte_order_mark(doc, &pos);

               /* Do this before parsing the XML declaration so the document position can be maintained. */
               doc->next_element = 2;    /* avoid -1, 0, false, true which can all accidentally happen */
               new_current_element(doc, 0, &doc->text[pos], 0, &doc->text[pos], 0, tag_none);

               parse_xml_declaration(doc, doc->current_element);
               if (doc->error == ALI_ERROR_NOT_XML_DOCUMENT &&
                  !(options & ALI_OPTION_INPUT_XML_DECLARATION))
               {
                  /* The option was not set to require a XML declaration. */
                  doc->error = ALI_ERROR_NONE;
               }

               if (!encoding_supported(doc->encoding))
               {
                  doc->error = ALI_ERROR_ENCODING_UNSUPPORTED;
               }

               if (doc->error == ALI_ERROR_NONE)
               {
                  /* Move the element name for the root to after the XML declaration.
                   * The code to find markups doesn't know how to handle the declaration,
                   * so just skip it here. */
                  doc->current_element->name = doc->current_element->pos;
                  doc->current_element->length = 0;
                  doc->current_element->last_markup_name = doc->current_element->pos;
                  doc->current_element->last_markup_name_length = 0;
               }
               result = doc->current_element->element;
            }
            else if (doc->size == 0)
               doc->error = ALI_ERROR_NOT_XML_DOCUMENT;
            else
               doc->error = ALI_ERROR_MEMORY_FAILURE;

            if (doc->file_in != NULL)
            {
               fclose(doc->file_in);
               doc->file_in = NULL;
            }
         }
         else
            doc->error = ALI_ERROR_FILE_MISSING;
      }
   }

   return result;
}




/*! \brief Close an Ali document.
 * 
 * Stop input from the Ali document.  Further use of the document is an error.
 * 
 * \see ali_open, ali_in */

void
ali_close(
   ali_doc_info * doc           /*!< the document to input from */
   )
{
   /* Check for doc usage after ali_close */
   assert(doc != NULL);
   assert(!(doc->text == NULL && doc->size > 0));

   /* Clearing this out guarantees that calling other APIs (ali_in(), ali_new_element()) is no
    * longer valid. */
   if (doc->elements_size > 0)
   {
      free(doc->elements);
      doc->elements = NULL;
      doc->elements_size = 0;
      doc->elements_count = 0;
   }

   if (doc->text != NULL)
   {
      memset((void *) doc->text, 0xfe, doc->size);
      free((void *) doc->text);
      doc->text = NULL;
   }
   
   free(doc);
}


/* this reads after the input variable for required characters.  These characters must follow the
 * variable read. */
static void
read_suffix(
   const char **format,
   char *suffix_str)
{
   const char *formatP = *format;
   char *suffix_end;

   assert(*format != NULL);
   assert(suffix_str != NULL);

   suffix_end = suffix_str;
   while (*formatP != '\0')
   {
      if (formatP[0] == '%')
      {
         if (formatP[1] == '%')
         {
            *suffix_end++ = '%';
            formatP += 2;
         }
         else
         {
            break;
         }
      }
      else if (formatP[0] == '^')
      {
         if (formatP[1] == '^')
         {
            *suffix_end++ = '^';
            formatP += 2;
         }
         else
         {
            break;
         }
      }
      else
         *suffix_end++ = *formatP++;;
   }

   *suffix_end++ = '\0';
}


static int
compare_markup_names(
    ali_doc_info * doc,
    const char *a, 
    const char *b, 
    int length)
{
    int result;

    /* The normal case is matching names. */
    result = strncmp(a, b, length);

    if (result != 0 &&
        doc->encoding == encoding_UTF_8 &&
        (doc->options & ALI_OPTION_EXP_CONVERT_UTF8_TO_ISO_8859_1))
    {
        /* By converting during comparing, we avoid having to 
         * modify a string, which means allocating, copying, and 
         * freeing. It also often avoids converting parts of 
         * strings not needed to decide the result. */
        result = 0;

        while (result == 0 && 
            length > 0 && 
            *a != '\0')
        {
            result = *a - *b;

            if (result != 0 &&
                !IS_UTF_8_SINGLE(*b))
            {
                if (*(unsigned char *)b == 0xC2)
                {
                    b++;
                    length--;
                    result = *a - *b;
                }
                else if (*(unsigned char *)b == 0xC3)
                {
                    b++;
                    length--;
                    result = *a - (*b + 64);
                }
            }

            a++;
            b++;
            length--;
        }
    }

    return result;
}


static ali_element_ref
parse_input_format(
   ali_doc_info * doc,
   ali_element_ref element,
   const char *format,
   va_list arg)
{
   const char *f;
   char prefix_str[32];         /* chars required before input variable */
   char suffix_str[32];         /* chars required after input variable */
   char *prefix_end = prefix_str;
   char *strP;
   char **strPP;
   bool element_optional = false;
   bool element_wanted = false; /* ^e or ^a wants an element */
   ali_element_ref result = false;
   int prefix;
   bool advance_to_content = false;    /* when reading content, 
                                        * advance to the start of the content 
                                        * for the markup type.  Set whenever 
                                        * markup is read, and left unset when
                                        * content is read successively. */


   /* Check for err */
   assert(doc != NULL);
   assert(format != NULL);

   /* Init vars. */
   f = format;

   /* Look for next format specification */
   while (*f)
   {

      if (*f == '^')
      {
         f++;
         element_optional = false;

         /* Attributes can be terminated by a '^'. 
          * '^' can be outputted by "^^". */

         /* Support "^e^a%s^a%s^%s^%s" */
         if (*f != '^')
         {
            advance_to_content = true;

            /* If closing a current command. */
            if (doc->current_element->last_markup_read != ali_element_none)
            {
               /* Don't close element in "^e^a". */
               if (!element_wanted)
               {
                  /* Done with element */
                  remove_markup(doc->current_element, doc->current_element->last_markup_read);
               }
            }
            /* If not a new command then pop up a level. */
            else if (*f != 'e' && *f != 'a' && *f != 'o' && *f != '*')
            {
               /* Support "^e^a%s^a%s^%s^%s" by closing the element */
               if (doc->current_element->element != element)
               {
                  delete_element(doc, &doc->current_element);
               }
               doc->current_element->data_used = true;

               result = true;
            }

            /* Now that it is closed, continue on to the 
             * format processing. */
            if (*f == '%')
                continue;

         }


         if (*f == 'o')
         {
            element_optional = true;
            if (*(f + 1) != 0)
               f++;
         }

         if (*f == 'e' || *f == 'a' || *f == 'C' || *f == 'P' || *f == '*')
         {
            ali_element_info *current_element;
            uint8_t markup_type;
            int8_t i = 0;
            bool element_found = false;
            bool read_prefix = false;

            if (*f == 'e')
            {
               markup_type = tag_element;
               read_prefix = true;
            }
            else if (*f == 'a')
            {
               markup_type = tag_attribute;
               read_prefix = true;
            }
            else if (*f == 'C')
               markup_type = tag_comment;
            else if (*f == 'P')
               markup_type = tag_instruction;

            if (read_prefix)
            {
               /* namespaces currently are not used.  0 means don't care, and is the only value
                * allowed. */
               prefix = va_arg(arg, int);
               
               if (prefix != 0)
               {
                  /* Make this problem really obvious for now */
                  doc->error = ALI_ERROR_NAMESPACE_INVALID;
                  assert(prefix == 0);
                  return false;
               }
            }


            if (*f != '*' && markup_type != tag_comment)
            {
               strP = va_arg(arg, char *);
               if (strP == NULL)
               {
                  doc->error = ALI_ERROR_NULL_TAG;
                  return false;
               }
            }
            

            current_element = doc->current_element;
#ifdef READ_ALL_ELEMENTS
            if (!current_element->elements_read)
            {
               read_all_element_tags(current_element);
               current_element->elements_read = true;
            }
#endif
            element_wanted = true;

            /* Find the element. Check the next one in the document first. Then
             * check all already seen and not used.  Then read the document 
             * until there are no more left in the current element. */
            if (current_element->count == 0 && !current_element->elements_read)
            {
               current_element->elements_read = !read_one_markup(current_element);
            }
            for (i = 0; i < current_element->count;)
            {
               if (*f == '*' ||
                  (current_element->markup_type[i] == markup_type &&
                     (markup_type == tag_comment ||
	                  compare_markup_names(doc, strP, current_element->markup_name[i], 
                        current_element->markup_length[i]) == 0)))
               {
                  element_found = true;
                  doc->current_element->last_markup_read = i;

                  /* Add a new element structure.  Since this is costly, do not do this for the
                   * common "^e%[sd]" cases.  Do do this for "^e" and "^e%F" and "^e^a%s^%s" */
                  if ((current_element->markup_type[i] == tag_element || *f == '*') &&
                      (f[1] == '\0' || f[1] == '^' || (f[1] == '%' && f[2] == 'F')))
                  {
                     /* Fixup position, which can be off if returning to an element. */
                     doc->current_element->pos =
                        &doc->current_element->markup_name[i][doc->current_element->
                        markup_length[i]];

                     new_current_element(doc, doc->current_element->element, doc->current_element->pos,
                        doc->current_element->line_number, doc->current_element->markup_name[i],
                        doc->current_element->markup_length[i], doc->current_element->markup_type[i]);

                     current_element = doc->current_element;

                     result = true;
                  }

                  /* DOLATER and make ali_get_markup_name() and make ali_get_markup_path() and make 
                   * ali_get_markup_type() */
                  break;
               }

               i++;

               if (i >= current_element->count && !current_element->elements_read)
               {
                  current_element->elements_read = !read_one_markup(current_element);
               }
            }
            if (!element_found)
            {
               /* find more elements or fail. */
               current_element->data_unavailable = true;

               /* Not found, report an error if the element is required and this is the first pass
                * of the parse function. */
               if (!element_optional && current_element->new_element && *f != '*')
               {
                  doc->error = ALI_ERROR_TAG_MISSING;
                  return false;
               }
               else
               {
                  /* when the format is "^oe%F" make sure the %F is not attempted */
                  while (*f != '\0' && *f != '^')
                     f++;

                  continue;
               }
            }

            /* Start collecting the prefix string */
            prefix_end = prefix_str;
         }
         else
         {
            /* Unrecognized character. */
            doc->error = ALI_ERROR_UNKNOWN_XML_INSTRUCTION;
            return false;
         }
         f++;
      }
      else
      {
         if (*f == '%')
         {
            bool long_arg = false;
            bool short_arg = false;
            bool long_double_arg = false;
            bool byte_arg = false;
            ali_element_function *callback;
            uint32_t width = 0;
            bool allocate = false;

            f++;                /* skip '%' */

            if (!doc->current_element->start_tag_closed)
               advance_to_content = true;

            for (;;)
            {
               if (*f == 'l')
               {
                  long_arg = true;
               }
               else if (*f == 'q' || *f == 'L')
               {
                  long_double_arg = true;
               }
               else if (*f == 'h')
               {
                  /* %h reads a short %hh reads a byte */
                  if (!long_arg && *(f - 1) == 'h')
                     byte_arg = true;
                  else
                     short_arg = true;
               }
               else if (*f == 'a')
               {
                  allocate = true;
               }
               else if (*f == 'F')
               {
                  callback = va_arg(arg, ali_element_function *);

                  if (!doc->current_element->elements_read || !element_wanted)
                  {
                     ali_element_ref new_element;
                     ali_element_info *next;


                     new_element = doc->current_element->element;

                     /* Repeatedly call the callback as long as it uses at least one element. This
                      * allows it to parse repeated elements until there are no more. */
                     do
                     {
                        if (doc->current_element->element == new_element)
                        {
                           doc->current_element->data_used = false;
                           doc->current_element->data_unavailable = false;
                        }

                        callback(doc, new_element, doc->data);

                        doc->current_element->new_element = false;
                     }
                     /* while elements has data to use. */
                     while (!ali_is_element_done(doc, doc->current_element->element));

                     /* This code skips past the element so that the next markup read does not need 
                      * to reskip the same data.  Normally reading markup starts at the end of the
                      * last markup read. */
                     skip_end_tag(doc->current_element);

                     /* This is a poor determinant of the "parent" element, since there is no data
                      * tracking this.  When it's wrong, it misses opportunities to skip elements,
                      * so it's a performance issue. */
                     if (doc->current_element > doc->elements)
                        next = doc->current_element - 1;
                     else
                        next = NULL;

                     if (next->pos == &doc->current_element->name[doc->current_element->length])
                     {
                        next->pos = doc->current_element->pos;
                     }
                     if (next->last_markup_name == doc->current_element->name)
                     {
                        next->last_markup_name = doc->current_element->pos;
                        next->type_of_last_markup_read = tag_none;
                     }
                     delete_element(doc, &doc->current_element);


                     /* Done with element */
                     doc->current_element->data_used = true;

                     element_wanted = false;
                     result = true;
                  }
                  break;
               }
               else if (*f == 's')
               {
                  char * content;

                  if (allocate)
                      strPP = va_arg(arg, char **);
                  else
                      strP = va_arg(arg, char *);

                  /* Do not clear the arg if not used. Otherwise this can clear already read data
                   * if ali loops because more markup exists. */

                  /* Done finding prefix chars. */
                  *prefix_end++ = '\0';

                  if (doc->current_element->last_markup_read != ali_element_none || !element_wanted)
                  {

                     /* Find the suffix chars. */
                     f++;
                     read_suffix(&f, suffix_str);
                     f--;


                     content =
                        get_content(doc->current_element,
                        advance_to_content,
                        prefix_str,
                        suffix_str, 
                        allocate ? NULL : strP, 
                        width);
                     if (allocate) 
                        *strPP = content;

                     if (doc->current_element->last_markup_read != ali_element_none)
                     {
                        /* Done with element */
                        doc->current_element->data_used = true;
                     }

                     element_wanted = false;
                     result = true;
                  }


                  break;
               }
               /* %eEfgG */
               else if (*f == 'e' || *f == 'E' || *f == 'f' || *f == 'g' || *f == 'G')
               {
                  char conversion = *f;
                  char number[32];          /* use stack space for number extraction and conversion. */
                  char sscanf_format[8];
                  char *sscanf_format_end;
                  long double * num_long_double;
                  double * num_double;
                  float * num_float;

                  
                  if (long_double_arg)
                  {
                     num_long_double = va_arg(arg, long double *);
                  }
                  else if (long_arg)
                  {
                     num_double = va_arg(arg, double *);
                  }
                  else
                  {
                     num_float = va_arg(arg, float *);
                  }

                  /* Done finding prefix chars. */
                  *prefix_end++ = '\0';

                  if (doc->current_element->last_markup_read != ali_element_none || !element_wanted)
                  {
                     /* Find the suffix chars. */
                     f++;
                     read_suffix(&f, suffix_str);
                     f--;


                     number[0] = '\0';
                     get_content(doc->current_element,
                        advance_to_content,
                        prefix_str, suffix_str, number, 
                        (width > 0 && width < sizeof(number) - 1) ? width : sizeof(number) - 1);

                     sscanf_format_end = sscanf_format;
                     *sscanf_format_end++ = '%';
                     if (long_double_arg)
                        *sscanf_format_end++ = 'L';
                     else if (long_arg)
                        *sscanf_format_end++ = 'l';
                     *sscanf_format_end++ = conversion;
                     *sscanf_format_end++ = '\0';

                     /* Handle if not an empty element */
                     if (number[0] != '\0')
                     {
                        if (long_double_arg)
                           sscanf_s(number, sscanf_format, num_long_double);
                        else if (long_arg)
                           sscanf_s(number, sscanf_format, num_double);
                        else
                           sscanf_s(number, sscanf_format, num_float);
                     }
                     else if (!element_optional && doc->current_element->new_element)
                     {
                        /* A valid value was expected for this element and there isn't any, so
                         * return an error */
                        doc->error = ALI_ERROR_CONTENT_MISSING;
                        return false;
                     }

                     /* Done with element */
                     doc->current_element->data_used = true;

                     element_wanted = false;
                     result = true;
                  }
                  break;
               }
               /* %diouXx */
               else if (*f == 'c' || *f == 'd' || *f == 'i' || *f == 'u'  || *f == 'o' || *f == 'x' || *f == 'X')
               {
                  char conversion = *f;
                  char number[16];          /* use stack space for number extraction and conversion. */
                  char sscanf_format[8];
                  char *sscanf_format_end;
                  long int *num_long;
                  short int *num_short;
                  int *num_int;
                  char *num_char;

                  if (long_arg)
                  {
                     num_long = va_arg(arg, long *);
                  }
                  else if (short_arg)
                  {
                     num_short = va_arg(arg, short *);
                  }
                  else if (conversion == 'c' || byte_arg)
                  {
                     num_char = va_arg(arg, char *);

                     /* limit get_content() to read only one char. */
                     if (width == 0)
                        width = 1;
                  }
                  else
                  {
                     num_int = va_arg(arg, int *);
                  }

                  /* Done finding prefix chars. */
                  *prefix_end++ = '\0';

                  if (doc->current_element->last_markup_read != ali_element_none || !element_wanted)
                  {
                     /* Find the suffix chars. */
                     f++;
                     read_suffix(&f, suffix_str);
                     f--;


                     number[0] = '\0';
                     get_content(doc->current_element,
                        advance_to_content,
                        prefix_str, suffix_str, number, 
                        (width > 0 && width < sizeof(number) - 1) ? width : sizeof(number) - 1);

                     sscanf_format_end = sscanf_format;
                     *sscanf_format_end++ = '%';
                     if (long_arg)
                        *sscanf_format_end++ = 'l';
                     if (short_arg)
                        *sscanf_format_end++ = 'h';
                     if (byte_arg)
                     {
                        *sscanf_format_end++ = 'h';
                        *sscanf_format_end++ = 'h';
                     }
                     *sscanf_format_end++ = conversion;
                     *sscanf_format_end++ = '\0';

                     /* Handle if not an empty element */
                     if (number[0] != '\0')
                     {
                        if (long_arg)
                           sscanf_s(number, sscanf_format, num_long);
                        else if (short_arg)
                           sscanf_s(number, sscanf_format, num_short);
                        else if (conversion == 'c' || byte_arg)
                           sscanf_s(number, sscanf_format, num_char);
                        else
                           sscanf_s(number, sscanf_format, num_int);
                     }
                     else if (!element_optional && doc->current_element->new_element)
                     {
                        /* A valid value was expected for this element and there isn't any, so
                         * return an error */
                        doc->error = ALI_ERROR_CONTENT_MISSING;
                        return false;
                     }

                     /* Done with element */
                     doc->current_element->data_used = true;

                     element_wanted = false;
                     result = true;
                  }

                  /* sscanf_s it */
                  break;
               }
               /* return the markup's path */
               else if (*f == 'p')
               {
                  char * content;

                  if (allocate)
                      strPP = va_arg(arg, char **);
                  else
                      strP = va_arg(arg, char *);

                  /* Do not clear the arg if not used. Otherwise this can clear already read data
                   * if ali loops because more markup exists. */

                  /* Done finding prefix chars. */
                  *prefix_end++ = '\0';

                  content = get_element_path(doc, doc->current_element->element);

                  if (allocate) 
                     *strPP = content;
                  else
                  {
                     if (width == 0)
                        strcpy_s(strP, strlen(strP), content);
                     else
                        strncpy_s(strP, strlen(strP), content, width);

                     free(content);
                  }

                  element_wanted = false;
                  result = true;


                  break;
               }
               else if ('0' <= *f && *f <= '9')
               {
                   width = width * 10 + (*f - '0');
               }
               else if (*f == '%')
               {
                  /* add the character to those required before the input */
                  *prefix_end++ = *f;
                  break;
               }
               else if (*f == '\0')
               {
                  /* This is an abrupt end. 
                   * add the character to those required before the input */
                  *prefix_end++ = *f;
                  break;
               }
               else
               {
               }
               f++;
            }
            
            prefix_end = prefix_str;


            if (*f != 'p')
               advance_to_content = false;

            /* Skip past the last char passed to fprintf. */
            if (*f != '\0')
               f++;

         }
         else
         {
            /* add the character to those required before the input */
            *prefix_end++ = *f++;
         }
      }


   }

   /* If closing a current command. */
   /* Don't close element in "^e^a". */
   if (doc->current_element->last_markup_read != ali_element_none && !element_wanted)
   {
      /* Done with element */
      remove_markup(doc->current_element, doc->current_element->last_markup_read);
   }

   if (result)
      result = doc->current_element->element;

   return result;
}

/*! \brief Input some XML formatted data.
 * 
 * Input data from doc at element according to the format 
 * specified.  The format is a C string containing text, XML 
 * instructions to read XML structures, and scanf like 
 * rules to input the document text and save in the arguments.
 * 
 * Text is input as is, except the XML entities for &<>\" are converted.
 * 
 * The syntax of an XML instruction is:
 * 
 * 
 * format is a string that consists of a repeatable set that 
 * lists what data to find and then parsing instructions: 
 * [XML Instructions[Parsing Instructions]*]*
 * 
 * XML Instructions = '^' [o]?[eac] 
 * - '^' start of XML instruction
 * - o finding this data is optional.  Without this, missing data 
 *     results in ALI_ERROR_TAG_MISSING unless this is not the first pass.
 * - e find an element matching the name in the next two arguments.  
 *     The first argument is for namespaces which are not supported yet.  Pass NULL.
 *     The second argument is for the local part.  Pass the entire qualified name.
 * - a find an attribute matching the name in the next two arguments.  
 *     The first argument is for namespaces which are not supported yet.  Pass NULL.
 *     The second argument is for the local part.  Pass the entire qualified name.
 * - c find the current element's content. 
 * - C find any comment
 * - P find a processing instruction matching the next argument
 * - * find any markup
 * 
 * If a XML instruction is not matched (because the document contains no matching XML data) 
 * then the following parsing instruction is skipped.
 * 
 * Parsing Instructions = prefix_chars*%[lh]?[a]?[width]?[scdiux]suffix_chars* 
 * - prefix_chars zero or more valid XML content chars required before the 
 *   data.  Any whitespace can replace any whitespace 
 * - l the data size is long 
 * - h the data size is short (half) 
 * - hh the data size is half a short (char)
 * - a allocate the string (for "%as" pass (char **))
 * - s parse the data as a string and store in the next argument.
 *     It is important to either specify a max string width (excluding the terminating
 *     nul char) or use the 'a' modifier (GNU scanf extension) to allocate appropriately
 *     sized storage.
 * - c parse the data as a char and store in the next argument.  Width not supported.
 * - d parse the data as a signed decimal integer and store in the next argument 
 * - i parse the data as a signed decimal, hex, or octal integer and store in the next argument 
 * - u parse the data as an unsigned decimal integer and store in the next argument 
 * - xX parse the data as a hexadecimal number and store in the next argument 
 * - o parse the data as an unsigned octal integer and store in the next argument 
 * - eEfgG parse the data as a floating point number and store in the next argument 
 * - F parse the data as an XML element using the function callback provided in the next argument.
 *     This is useful to decompose that code into a separate function.
 * - p store the XPath (string) of the markup in the next argument, i.e. "^*%ap%as".
 * - suffix_chars zero or more valid XML content chars required after the 
 * data.  Any whitespace can replace any whitespace
 * 
 * If the element is not the current element, then the elements are closed 
 * until the correct element is reached.  If the correct element is not reached, 
 * then DOLATER.
 * 
 * \return The ali_element_ref of the element being read, or 0.
 * 
 * \see ali_open, ali_close, ali_is_element_new, ali_get_error */

ali_element_ref
ali_in(
   ali_doc_info * doc,          /*!< the document to input from */
   ali_element_ref element,     /*!< which XML element in the document to input from */
   const char *format,          /*!< C string containing the text and XML instructions to input.
                                 * This indicates how to interpret all following arguments. */
   ...)
{
   va_list args;
   ali_element_ref result = false;


   /* Check for err */
   assert(doc != NULL);
   assert(format != NULL);
   assert(element != 1);        /* Invalid element ref */

   if (doc->error == ALI_ERROR_NONE)
   {
       /* Search for the element specified. */
       if (doc->current_element->element != element)
       {

           if (find_element(doc, element) != NULL)
           {
              while (element < doc->current_element->element)
              {
                 delete_element(doc, &doc->current_element);
              }

              assert(doc->current_element->element == element);
           }
       }
       
       if (doc->current_element == NULL)
       {
           /* DOLATER error if invalid element? (element != 0) */
           return false;
           
       }
       
       
       if (!setjmp(doc->environment))
       {
           va_start(args, format);
           result = parse_input_format(doc, element, format, args);
           va_end(args);
       }
   }

   return result;
}


/*! \brief Get the error status of an Ali document.
 * 
 * A successfully read document will always have the status ALI_ERROR_NONE.  Once
 * the status is set to another code, it will remain set until the document is closed.
 *
 * \return An ali_error code like ALI_ERROR_NONE.
 * 
 * \see ali_in, ali_set_error */

ali_error
ali_get_error(
   ali_doc_info * doc           /*!< the document to input from */
   )
{
   /* Check for doc usage after ali_close */
   assert(doc != NULL);
   assert(!(doc->text == NULL && doc->size > 0));

   return doc->error;
}


/*! \brief Set the error status of an Ali document.
 * 
 * Often used by the app to set the error status to ALI_ERROR_MEMORY_FAILURE when it cannot
 * allocate neccesary structures to store the XML document's data.
 * 
 * \see ali_in, ali_get_error */

void
ali_set_error(
   ali_doc_info * doc,           /*!< the document to input from */
   ali_error new_error)          /*!< the new error code to use. */
{
   /* Check for doc usage after ali_close */
   assert(doc != NULL);
   assert(!(doc->text == NULL && doc->size > 0));

   doc->error = new_error;
}


/* Guiding Principles 
 * - minimize callback functions (only for nodes, not leaves). 
 * - procedure code is easier to write. 
 * - skipping elements is fundamental, so support it
 * 
 * The input and code are not guaranteed to be in the same order so 
 * either the code is called in the input order or the input is read 
 * in the code order.
 * 
 * Using callbacks means many function creations and API explosion. 
 * Environment passing is also tricky.
 * 
 * Reading input based on the element name means a single API.  Environment 
 * setup is comparitively trivial.
 * 
 * Callbacks are normally done to minimize memory usage because only 
 * memory for the current element is needed.  But in-order reading 
 * is the same and jumbled order can be almost the same.  Both worst 
 * cases, all elements depend on elements after themselves, are the same, 
 * except callbacks force temporary storage of dependencies whereas 
 * reading input can defer the element.
 * 
 * 
 * Type information resides in both the code and schema.  Until the 
 * language is changed, that's life.  At least a scheme validator can 
 * detect discrepencies indicating that a manual code change is needed. */
