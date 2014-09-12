// Copyright 2006 Nemanja Trifunovic
// Distributed under BOOST software license version 1.0

#pragma once

#include <stdexcept>
#include <iterator>

namespace utf8
{
    // The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
    // You may need to change them to match your system.
    // These typedefs have the same names as ones from cstdint, or boost/cstdint
    typedef unsigned char   uint8_t;
    typedef unsigned short  uint16_t;
    typedef unsigned int    uint32_t;

// Helper code - not intended to be directly called by the library users. May be changed at any time
namespace internal
{
    // Unicode constants
    // Leading (high) surrogates: 0xd800 - 0xdbff
    // Trailing (low) surrogates: 0xdc00 - 0xdfff
    const uint16_t LEAD_SURROGATE_MIN  = 0xd800u;
    const uint16_t LEAD_SURROGATE_MAX  = 0xdbffu;
    const uint16_t TRAIL_SURROGATE_MIN = 0xdc00u;
    const uint16_t TRAIL_SURROGATE_MAX = 0xdfffu;
    const uint16_t LEAD_OFFSET         = LEAD_SURROGATE_MIN - (0x10000 >> 10);
    const uint32_t SURROGATE_OFFSET    = 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN;

    // Maximum valid value for a Unicode code point
    const uint32_t CODE_POINT_MAX      = 0x0010ffffu;

    template<typename octet_type>
    inline uint8_t mask8(octet_type oc)
    {
        return static_cast<uint8_t>(0xff & oc);
    }
    template<typename u16_type>
    inline uint16_t mask16(u16_type oc)
    {
        return static_cast<uint16_t>(0xffff & oc);
    }
    template<typename octet_type>
    inline bool is_trail(octet_type oc)
    {
        return ((utf8::internal::mask8(oc) >> 6) == 0x2);
    }

    template <typename u16>
    inline bool is_lead_surrogate(u16 cp)
    {
        return (cp >= LEAD_SURROGATE_MIN && cp <= LEAD_SURROGATE_MAX);
    }

    template <typename u16>
    inline bool is_trail_surrogate(u16 cp)
    {
        return (cp >= TRAIL_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
    }

    template <typename u16>
    inline bool is_surrogate(u16 cp)
    {
        return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
    }

    template <typename u32>
    inline bool is_code_point_valid(u32 cp)
    {
        return (cp <= CODE_POINT_MAX && !utf8::internal::is_surrogate(cp));
    }

    template <typename octet_iterator>
    inline typename std::iterator_traits<octet_iterator>::difference_type
    sequence_length(octet_iterator lead_it)
    {
        uint8_t lead = utf8::internal::mask8(*lead_it);
        if (lead < 0x80)
            return 1;
        else if ((lead >> 5) == 0x6)
            return 2;
        else if ((lead >> 4) == 0xe)
            return 3;
        else if ((lead >> 3) == 0x1e)
            return 4;
        else
            return 0;
    }

    template <typename octet_difference_type>
    inline bool is_overlong_sequence(uint32_t cp, octet_difference_type length)
    {
        if (cp < 0x80) {
            if (length != 1) 
                return true;
        }
        else if (cp < 0x800) {
            if (length != 2) 
                return true;
        }
        else if (cp < 0x10000) {
            if (length != 3) 
                return true;
        }

        return false;
    }

    enum utf_error {UTF8_OK, NOT_ENOUGH_ROOM, INVALID_LEAD, INCOMPLETE_SEQUENCE, OVERLONG_SEQUENCE, INVALID_CODE_POINT};

    /// Helper for get_sequence_x
    template <typename octet_iterator>
    utf_error increase_safely(octet_iterator& it, octet_iterator end)
    {
        if (++it == end)
            return NOT_ENOUGH_ROOM;

        if (!utf8::internal::is_trail(*it))
            return INCOMPLETE_SEQUENCE;
        
        return UTF8_OK;
    }

    #define UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(IT, END) {utf_error ret = increase_safely(IT, END); if (ret != UTF8_OK) return ret;}    

    /// get_sequence_x functions decode utf-8 sequences of the length x
    template <typename octet_iterator>
    utf_error get_sequence_1(octet_iterator& it, octet_iterator end, uint32_t& code_point)
    {
        if (it == end)
            return NOT_ENOUGH_ROOM;

        code_point = utf8::internal::mask8(*it);

        return UTF8_OK;
    }

    template <typename octet_iterator>
    utf_error get_sequence_2(octet_iterator& it, octet_iterator end, uint32_t& code_point)
    {
        if (it == end) 
            return NOT_ENOUGH_ROOM;
        
        code_point = utf8::internal::mask8(*it);

        UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

        code_point = ((code_point << 6) & 0x7ff) + ((*it) & 0x3f);

        return UTF8_OK;
    }

    template <typename octet_iterator>
    utf_error get_sequence_3(octet_iterator& it, octet_iterator end, uint32_t& code_point)
    {
        if (it == end)
            return NOT_ENOUGH_ROOM;
            
        code_point = utf8::internal::mask8(*it);

        UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

        code_point = ((code_point << 12) & 0xffff) + ((utf8::internal::mask8(*it) << 6) & 0xfff);

        UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

        code_point += (*it) & 0x3f;

        return UTF8_OK;
    }

    template <typename octet_iterator>
    utf_error get_sequence_4(octet_iterator& it, octet_iterator end, uint32_t& code_point)
    {
        if (it == end)
           return NOT_ENOUGH_ROOM;

        code_point = utf8::internal::mask8(*it);

        UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

        code_point = ((code_point << 18) & 0x1fffff) + ((utf8::internal::mask8(*it) << 12) & 0x3ffff);

        UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

        code_point += (utf8::internal::mask8(*it) << 6) & 0xfff;

        UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

        code_point += (*it) & 0x3f;

        return UTF8_OK;
    }

    #undef UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR

    template <typename octet_iterator>
    utf_error validate_next(octet_iterator& it, octet_iterator end, uint32_t& code_point)
    {
        // Save the original value of it so we can go back in case of failure
        // Of course, it does not make much sense with i.e. stream iterators
        octet_iterator original_it = it;

        uint32_t cp = 0;
        // Determine the sequence length based on the lead octet
        typedef typename std::iterator_traits<octet_iterator>::difference_type octet_difference_type;
        const octet_difference_type length = utf8::internal::sequence_length(it);

        // Get trail octets and calculate the code point
        utf_error err = UTF8_OK;
        switch (length) {
            case 0: 
                return INVALID_LEAD;
            case 1:
                err = utf8::internal::get_sequence_1(it, end, cp);
                break;
            case 2:
                err = utf8::internal::get_sequence_2(it, end, cp);
            break;
            case 3:
                err = utf8::internal::get_sequence_3(it, end, cp);
            break;
            case 4:
                err = utf8::internal::get_sequence_4(it, end, cp);
            break;
        }

        if (err == UTF8_OK) {
            // Decoding succeeded. Now, security checks...
            if (utf8::internal::is_code_point_valid(cp)) {
                if (!utf8::internal::is_overlong_sequence(cp, length)){
                    // Passed! Return here.
                    code_point = cp;
                    ++it;
                    return UTF8_OK;
                }
                else
                    err = OVERLONG_SEQUENCE;
            }
            else 
                err = INVALID_CODE_POINT;
        }

        // Failure branch - restore the original value of the iterator
        it = original_it;
        return err;
    }

    template <typename octet_iterator>
    inline utf_error validate_next(octet_iterator& it, octet_iterator end) {
        uint32_t ignored;
        return utf8::internal::validate_next(it, end, ignored);
    }

} // namespace internal

    /// The library API - functions intended to be called by the users

    // Byte order mark
    const uint8_t bom[] = {0xef, 0xbb, 0xbf};

    template <typename octet_iterator>
    octet_iterator find_invalid(octet_iterator start, octet_iterator end)
    {
        octet_iterator result = start;
        while (result != end) {
            utf8::internal::utf_error err_code = utf8::internal::validate_next(result, end);
            if (err_code != internal::UTF8_OK)
                return result;
        }
        return result;
    }

    template <typename octet_iterator>
    inline bool is_valid(octet_iterator start, octet_iterator end)
    {
        return (utf8::find_invalid(start, end) == end);
    }

    template <typename octet_iterator>
    inline bool starts_with_bom (octet_iterator it, octet_iterator end)
    {
        return (
            ((it != end) && (utf8::internal::mask8(*it++)) == bom[0]) &&
            ((it != end) && (utf8::internal::mask8(*it++)) == bom[1]) &&
            ((it != end) && (utf8::internal::mask8(*it))   == bom[2])
           );
    }
    
    //Deprecated in release 2.3 
    template <typename octet_iterator>
    inline bool is_bom (octet_iterator it)
    {
        return (
            (utf8::internal::mask8(*it++)) == bom[0] &&
            (utf8::internal::mask8(*it++)) == bom[1] &&
            (utf8::internal::mask8(*it))   == bom[2]
           );
    }

    //=============================================================================================
    // checked
    //=============================================================================================

    // Base for the exceptions that may be thrown from the library
    class exception : public ::std::exception {
    };

    // Exceptions that may be thrown from the library functions.
    class invalid_code_point : public exception {
        uint32_t cp;
    public:
        invalid_code_point(uint32_t cp) : cp(cp) {}
        virtual const char* what() const throw() { return "Invalid code point"; }
        uint32_t code_point() const {return cp;}
    };

    class invalid_utf8 : public exception {
        uint8_t u8;
    public:
        invalid_utf8 (uint8_t u) : u8(u) {}
        virtual const char* what() const throw() { return "Invalid UTF-8"; }
        uint8_t utf8_octet() const {return u8;}
    };

    class invalid_utf16 : public exception {
        uint16_t u16;
    public:
        invalid_utf16 (uint16_t u) : u16(u) {}
        virtual const char* what() const throw() { return "Invalid UTF-16"; }
        uint16_t utf16_word() const {return u16;}
    };

    class not_enough_room : public exception {
    public:
        virtual const char* what() const throw() { return "Not enough space"; }
    };

    /// The library API - functions intended to be called by the users

    template <typename octet_iterator>
    octet_iterator append(uint32_t cp, octet_iterator result)
    {
        if (!utf8::internal::is_code_point_valid(cp))
            throw invalid_code_point(cp);

        if (cp < 0x80)                        // one octet
            *(result++) = static_cast<uint8_t>(cp);
        else if (cp < 0x800) {                // two octets
            *(result++) = static_cast<uint8_t>((cp >> 6)            | 0xc0);
            *(result++) = static_cast<uint8_t>((cp & 0x3f)          | 0x80);
        }
        else if (cp < 0x10000) {              // three octets
            *(result++) = static_cast<uint8_t>((cp >> 12)           | 0xe0);
            *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f)   | 0x80);
            *(result++) = static_cast<uint8_t>((cp & 0x3f)          | 0x80);
        }
        else {                                // four octets
            *(result++) = static_cast<uint8_t>((cp >> 18)           | 0xf0);
            *(result++) = static_cast<uint8_t>(((cp >> 12) & 0x3f)  | 0x80);
            *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f)   | 0x80);
            *(result++) = static_cast<uint8_t>((cp & 0x3f)          | 0x80);
        }
        return result;
    }

    template <typename octet_iterator, typename output_iterator>
    output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out, uint32_t replacement)
    {
        while (start != end) {
            octet_iterator sequence_start = start;
            internal::utf_error err_code = utf8::internal::validate_next(start, end);
            switch (err_code) {
                case internal::UTF8_OK :
                    for (octet_iterator it = sequence_start; it != start; ++it)
                        *out++ = *it;
                    break;
                case internal::NOT_ENOUGH_ROOM:
                    throw not_enough_room();
                case internal::INVALID_LEAD:
                    out = utf8::append (replacement, out);
                    ++start;
                    break;
                case internal::INCOMPLETE_SEQUENCE:
                case internal::OVERLONG_SEQUENCE:
                case internal::INVALID_CODE_POINT:
                    out = utf8::append (replacement, out);
                    ++start;
                    // just one replacement mark for the sequence
                    while (start != end && utf8::internal::is_trail(*start))
                        ++start;
                    break;
            }
        }
        return out;
    }

    template <typename octet_iterator, typename output_iterator>
    inline output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out)
    {
        static const uint32_t replacement_marker = utf8::internal::mask16(0xfffd);
        return utf8::replace_invalid(start, end, out, replacement_marker);
    }

    template <typename octet_iterator>
    uint32_t next(octet_iterator& it, octet_iterator end)
    {
        uint32_t cp = 0;
        internal::utf_error err_code = utf8::internal::validate_next(it, end, cp);
        switch (err_code) {
            case internal::UTF8_OK :
                break;
            case internal::NOT_ENOUGH_ROOM :
                throw not_enough_room();
            case internal::INVALID_LEAD :
            case internal::INCOMPLETE_SEQUENCE :
            case internal::OVERLONG_SEQUENCE :
                throw invalid_utf8(*it);
            case internal::INVALID_CODE_POINT :
                throw invalid_code_point(cp);
        }
        return cp;
    }

    template <typename octet_iterator>
    uint32_t peek_next(octet_iterator it, octet_iterator end)
    {
        return utf8::next(it, end);
    }

    template <typename octet_iterator>
    uint32_t prior(octet_iterator& it, octet_iterator start)
    {
        // can't do much if it == start
        if (it == start)
            throw not_enough_room();

        octet_iterator end = it;
        // Go back until we hit either a lead octet or start
        while (utf8::internal::is_trail(*(--it)))
            if (it == start)
                throw invalid_utf8(*it); // error - no lead byte in the sequence
        return utf8::peek_next(it, end);
    }

    /// Deprecated in versions that include "prior"
    template <typename octet_iterator>
    uint32_t previous(octet_iterator& it, octet_iterator pass_start)
    {
        octet_iterator end = it;
        while (utf8::internal::is_trail(*(--it)))
            if (it == pass_start)
                throw invalid_utf8(*it); // error - no lead byte in the sequence
        octet_iterator temp = it;
        return utf8::next(temp, end);
    }

    template <typename octet_iterator, typename distance_type>
    void advance (octet_iterator& it, distance_type n, octet_iterator end)
    {
        for (distance_type i = 0; i < n; ++i)
            utf8::next(it, end);
    }

    template <typename octet_iterator>
    typename std::iterator_traits<octet_iterator>::difference_type
    distance (octet_iterator first, octet_iterator last)
    {
        typename std::iterator_traits<octet_iterator>::difference_type dist;
        for (dist = 0; first < last; ++dist)
            utf8::next(first, last);
        return dist;
    }

    template <typename u16bit_iterator, typename octet_iterator>
    octet_iterator utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result)
    {
        while (start != end) {
            uint32_t cp = utf8::internal::mask16(*start++);
            // Take care of surrogate pairs first
            if (utf8::internal::is_lead_surrogate(cp)) {
                if (start != end) {
                    uint32_t trail_surrogate = utf8::internal::mask16(*start++);
                    if (utf8::internal::is_trail_surrogate(trail_surrogate))
                        cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
                    else
                        throw invalid_utf16(static_cast<uint16_t>(trail_surrogate));
                }
                else
                    throw invalid_utf16(static_cast<uint16_t>(cp));

            }
            // Lone trail surrogate
            else if (utf8::internal::is_trail_surrogate(cp))
                throw invalid_utf16(static_cast<uint16_t>(cp));

            result = utf8::append(cp, result);
        }
        return result;
    }

    template <typename u16bit_iterator, typename octet_iterator>
    u16bit_iterator utf8to16 (octet_iterator start, octet_iterator end, u16bit_iterator result)
    {
        while (start != end) {
            uint32_t cp = utf8::next(start, end);
            if (cp > 0xffff) { //make a surrogate pair
                *result++ = static_cast<uint16_t>((cp >> 10)   + internal::LEAD_OFFSET);
                *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
            }
            else
                *result++ = static_cast<uint16_t>(cp);
        }
        return result;
    }

    template <typename octet_iterator, typename u32bit_iterator>
    octet_iterator utf32to8 (u32bit_iterator start, u32bit_iterator end, octet_iterator result)
    {
        while (start != end)
            result = utf8::append(*(start++), result);

        return result;
    }

    template <typename octet_iterator, typename u32bit_iterator>
    u32bit_iterator utf8to32 (octet_iterator start, octet_iterator end, u32bit_iterator result)
    {
        while (start != end)
            (*result++) = utf8::next(start, end);

        return result;
    }

    // The iterator class
    template <typename octet_iterator>
    class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t> {
      octet_iterator it;
      octet_iterator range_start;
      octet_iterator range_end;
      public:
      iterator () {}
      explicit iterator (const octet_iterator& octet_it,
                         const octet_iterator& range_start,
                         const octet_iterator& range_end) :
               it(octet_it), range_start(range_start), range_end(range_end)
      {
          if (it < range_start || it > range_end)
              throw std::out_of_range("Invalid utf-8 iterator position");
      }
      // the default "big three" are OK
      octet_iterator base () const { return it; }
      uint32_t operator * () const
      {
          octet_iterator temp = it;
          return utf8::next(temp, range_end);
      }
      bool operator == (const iterator& rhs) const
      {
          if (range_start != rhs.range_start || range_end != rhs.range_end)
              throw std::logic_error("Comparing utf-8 iterators defined with different ranges");
          return (it == rhs.it);
      }
      bool operator != (const iterator& rhs) const
      {
          return !(operator == (rhs));
      }
      iterator& operator ++ ()
      {
          utf8::next(it, range_end);
          return *this;
      }
      iterator operator ++ (int)
      {
          iterator temp = *this;
          utf8::next(it, range_end);
          return temp;
      }
      iterator& operator -- ()
      {
          utf8::prior(it, range_start);
          return *this;
      }
      iterator operator -- (int)
      {
          iterator temp = *this;
          utf8::prior(it, range_start);
          return temp;
      }
    }; // class iterator

    //=============================================================================================
    // unchecked
    //=============================================================================================

    namespace unchecked 
    {
        template <typename octet_iterator>
        octet_iterator append(uint32_t cp, octet_iterator result)
        {
            if (cp < 0x80)                        // one octet
                *(result++) = static_cast<uint8_t>(cp);  
            else if (cp < 0x800) {                // two octets
                *(result++) = static_cast<uint8_t>((cp >> 6)          | 0xc0);
                *(result++) = static_cast<uint8_t>((cp & 0x3f)        | 0x80);
            }
            else if (cp < 0x10000) {              // three octets
                *(result++) = static_cast<uint8_t>((cp >> 12)         | 0xe0);
                *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
                *(result++) = static_cast<uint8_t>((cp & 0x3f)        | 0x80);
            }
            else {                                // four octets
                *(result++) = static_cast<uint8_t>((cp >> 18)         | 0xf0);
                *(result++) = static_cast<uint8_t>(((cp >> 12) & 0x3f)| 0x80);
                *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
                *(result++) = static_cast<uint8_t>((cp & 0x3f)        | 0x80);
            }
            return result;
        }

        template <typename octet_iterator>
        uint32_t next(octet_iterator& it)
        {
            uint32_t cp = utf8::internal::mask8(*it);
            typename std::iterator_traits<octet_iterator>::difference_type length = utf8::internal::sequence_length(it);
            switch (length) {
                case 1:
                    break;
                case 2:
                    it++;
                    cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
                    break;
                case 3:
                    ++it; 
                    cp = ((cp << 12) & 0xffff) + ((utf8::internal::mask8(*it) << 6) & 0xfff);
                    ++it;
                    cp += (*it) & 0x3f;
                    break;
                case 4:
                    ++it;
                    cp = ((cp << 18) & 0x1fffff) + ((utf8::internal::mask8(*it) << 12) & 0x3ffff);                
                    ++it;
                    cp += (utf8::internal::mask8(*it) << 6) & 0xfff;
                    ++it;
                    cp += (*it) & 0x3f; 
                    break;
            }
            ++it;
            return cp;        
        }

        template <typename octet_iterator>
        uint32_t peek_next(octet_iterator it)
        {
            return utf8::unchecked::next(it);    
        }

        template <typename octet_iterator>
        uint32_t prior(octet_iterator& it)
        {
            while (utf8::internal::is_trail(*(--it))) ;
            octet_iterator temp = it;
            return utf8::unchecked::next(temp);
        }

        // Deprecated in versions that include prior, but only for the sake of consistency (see utf8::previous)
        template <typename octet_iterator>
        inline uint32_t previous(octet_iterator& it)
        {
            return utf8::unchecked::prior(it);
        }

        template <typename octet_iterator, typename distance_type>
        void advance (octet_iterator& it, distance_type n)
        {
            for (distance_type i = 0; i < n; ++i)
                utf8::unchecked::next(it);
        }

        template <typename octet_iterator>
        typename std::iterator_traits<octet_iterator>::difference_type
        distance (octet_iterator first, octet_iterator last)
        {
            typename std::iterator_traits<octet_iterator>::difference_type dist;
            for (dist = 0; first < last; ++dist) 
                utf8::unchecked::next(first);
            return dist;
        }

        template <typename u16bit_iterator, typename octet_iterator>
        octet_iterator utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result)
        {       
            while (start != end) {
                uint32_t cp = utf8::internal::mask16(*start++);
            // Take care of surrogate pairs first
                if (utf8::internal::is_lead_surrogate(cp)) {
                    uint32_t trail_surrogate = utf8::internal::mask16(*start++);
                    cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
                }
                result = utf8::unchecked::append(cp, result);
            }
            return result;         
        }

        template <typename u16bit_iterator, typename octet_iterator>
        u16bit_iterator utf8to16 (octet_iterator start, octet_iterator end, u16bit_iterator result)
        {
            while (start < end) {
                uint32_t cp = utf8::unchecked::next(start);
                if (cp > 0xffff) { //make a surrogate pair
                    *result++ = static_cast<uint16_t>((cp >> 10)   + internal::LEAD_OFFSET);
                    *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
                }
                else
                    *result++ = static_cast<uint16_t>(cp);
            }
            return result;
        }

        template <typename octet_iterator, typename u32bit_iterator>
        octet_iterator utf32to8 (u32bit_iterator start, u32bit_iterator end, octet_iterator result)
        {
            while (start != end)
                result = utf8::unchecked::append(*(start++), result);

            return result;
        }

        template <typename octet_iterator, typename u32bit_iterator>
        u32bit_iterator utf8to32 (octet_iterator start, octet_iterator end, u32bit_iterator result)
        {
            while (start < end)
                (*result++) = utf8::unchecked::next(start);

            return result;
        }

        // The iterator class
        template <typename octet_iterator>
          class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t> { 
            octet_iterator it;
            public:
            iterator () {}
            explicit iterator (const octet_iterator& octet_it): it(octet_it) {}
            // the default "big three" are OK
            octet_iterator base () const { return it; }
            uint32_t operator * () const
            {
                octet_iterator temp = it;
                return utf8::unchecked::next(temp);
            }
            bool operator == (const iterator& rhs) const 
            { 
                return (it == rhs.it);
            }
            bool operator != (const iterator& rhs) const
            {
                return !(operator == (rhs));
            }
            iterator& operator ++ () 
            {
                ::std::advance(it, utf8::internal::sequence_length(it));
                return *this;
            }
            iterator operator ++ (int)
            {
                iterator temp = *this;
                ::std::advance(it, utf8::internal::sequence_length(it));
                return temp;
            }  
            iterator& operator -- ()
            {
                utf8::unchecked::prior(it);
                return *this;
            }
            iterator operator -- (int)
            {
                iterator temp = *this;
                utf8::unchecked::prior(it);
                return temp;
            }
          }; // class iterator

    } // namespace utf8::unchecked
} // namespace utf8 
