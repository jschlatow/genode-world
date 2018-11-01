/*
 * \brief  Resource record implementation for mDNS.
 * \author Nikolas Brendes
 * \date   2018-07-01
 */

#ifndef _INCLUDE_MDNS_RR_H_
#define _INCLUDE_MDNS_RR_H_

#include <base/env.h>
#include <base/heap.h>
#include <util/endian.h>
#include <net/ipv4.h>
#include <util/string.h>

namespace Net
{
    namespace mDNS
    {
		

        enum {
			STANDARD_TTL = 120,
			MAX_NAME_LEN = 255
		};
		using Name = Genode::String<MAX_NAME_LEN>;
        struct Type
        {   

			enum : Genode::uint16_t
			{
        		A 		= 1,	// host address
        		NS		= 2,	// authoritative name server
        		MD		= 3, 	// mail destination (Obsolete - use MX)
        		MF		= 4, 	// mail forwarder (Obsolete - use MX)
        		CNAME 	= 5,	// canonical name of an alias
        		SOA 	= 6, 	// marks the start of a zone of authority
        		MB 		= 7, 	// mailbox domain name
        		MG 		= 8, 	// mailbox group member
        		MR 		= 9, 	// mail rename domain name
        		NULL 	= 10,	// null RR
        		WKS 	= 11, 	// well known service description
        		PTR 	= 12, 	// domain name pointer
        		HINFO 	= 13, 	// host hardware information
        		MINFO 	= 14, 	// mailbox or mail list information
        		MX 		= 15, 	// mail exchange
        		TXT 	= 16	// text strings
			};
        };
        struct QType :  Type
        {
        	enum : Genode::uint16_t
			{
				AXFR 	= 252, 	// request for a transfer of an entrie zone
        		MAILB 	= 253, 	// request for mailbox-related records (MB, MG or MR)
        		MAILA 	= 254,	// request for mail agent RRs (Obsolete - see MX)
        		ANY		= 255 	// request for all records
			};
		};


        struct Class
        {
        	enum : Genode::uint16_t {
        		IN 		= 1, 	// the internet
        		CS 		= 2, 	// CSNET Class (Obsolete - used only for exampels in some obsolete RFCs)
        		CH 		= 3, 	// CHAOS Class
        		HS 		= 4 	// Hesiod
        	};
        };
        struct QClass : Class
        {
        	enum : Genode::uint16_t {
        		ANY 	= 255 	// any class 
        	};
        };
        
		/*
        class Domain_Name : public Genode::List<Domain_Name>::Element
        {
          private:
            void gen_labels(char * str)
        	{
        		int i = 0;
				while( !( *(ptr + i) == 46 || *(ptr + i) == 0)) 
				{
					i++;
				}
				if(i == 0)
				{
					gen_labels(ptr + 1);
					return;
				}
				labels[count].addr = ptr;
				labels[count].len = i;
				count++;
				if(*(ptr + i) != 0) 
					gen_labels(ptr + i + 1);
        	}

          public:  
            struct Label : public Genode::List<Label>::Element
            {
              public:
            	Genode::uint8_t 	_length;
            	char	            *_label;



            	Label() {} 

            	Label(Genode::uint8_t len, char* str_ptr) :  _length(len), _label(str_ptr)
            	{		
            	}
            };

        	
        	char 	        _name[255];
        	Genode::uint8_t _count;
            Genode::uint8_t _length;
        	Label  	        _labels[32];
        	Domain_Name() {}
        	Domain_Name(char * name) : _count(0), _length(0)
        	{
				for(int i = 0; i < 255; i++)
					_name[i] = 0;

        		if(name[0]<46)
        			Genode::strncpy(_name, name, 255);
        		else
        			Genode::strncpy(_name + 1, name, 255);
        		_length = Genode::strlen(_name);
        		gen_labels(_name);
        	}

        	void clear()
            {
                _length = 0;
                _count = 0;
                for(int i = 0; i < 255; i++)
                    _name[i] = 0;

                for(int i = 0; i < 32; i++)
                {
                    _labels[i]._label = nullptr;
                    _labels[i]._length = 0;
                }
            }

            bool match(Domain_Name dName)
            {
				return true;
                if(dName._count == _count)
                {
                    if(!Genode::strcmp(dName._labels[0]._label, _labels[0]._label, 255))
                        return true;
                }
                else if(dName._count > _count)
                {
                    if(!Genode::strcmp(dName._labels[dName._count - _count]._label, _labels[0]._label, 255))
                        return true;
                }
                else if( _count > dName._count)
                {
                    if(!Genode::strcmp(dName._labels[0]._label, _labels[_count - dName._count]._label, 255))
                        return true;
                }
                
                return false;
            }

            

        };
		*/

		/* FIXME remove/replace *_Ptr classes
		 *       The Domain_Name_Ptr is only a set of pointers to the domain name labels.
		 *       It must therefore be used only temporarily (with minimal scope).
		 */
		class Domain_Name_Ptr 
		{
			struct Label{
				const char		*addr = 0;
				Genode::uint8_t len = 0;
			};

			Label 			labels[32];
			Genode::uint8_t count = 0;

			void gen_labels(const char * ptr)
			{
				int i = 0;
				while( !( *(ptr + i) == 46 || *(ptr + i) == 0)) 
				{
					i++;
				}
				if(i == 0)
				{
					gen_labels(ptr + 1);
					return;
				}
				labels[count].addr = ptr;
				labels[count].len = i;
				count++;
				if(*(ptr + i) != 0) 
					gen_labels(ptr + i + 1);
				
			}

		  public:
			
			Domain_Name_Ptr(const char * name) 
			{
				if( *name > 45)
				{
					gen_labels(name);
				}
				else
				{
					int i = 0;
					while(* (name + i) != 0)
					{
						labels[count].addr = name + i + 1;
						labels[count].len = *(name + i);
						i += *(name + i) + 1;
						count++;
					}
				}
			}
			

			Genode::int8_t match(Domain_Name_Ptr d2, int label_offset = 0)
			{
				int i1 = count -1;
				int i2 = d2.count -1;
				
				while(!Genode::strcmp(labels[i1].addr, d2.labels[i2].addr, Genode::max(labels[i1].len,d2.labels[i2].len)))
				{
					i1--;
					i2--;
					if(i1 == i2)
					{
						if((i1 - label_offset) < 0 || (i2 - label_offset) < 0){	
							return 1;
						}
					}
					else if(i1 < 0 || i2 < 0)
					{
						return 0;
					}
				}
				return -1;
			}

			//Genode::size_t size()
			//{
			//	return Genode::strlen(labels[0].addr) + 2;
			//}

			Genode::size_t copy(char *dst)
			{
				Genode::size_t offset = 0;

				for(int i = 0; i < count; i++)
				{
					dst[offset] = labels[i].len;
					Genode::memcpy(dst + 1 + offset, labels[i].addr, labels[i].len);
					offset += labels[i].len + 1;	
				}
				dst[offset] = 0;
				return offset + 1;

			}
			Genode::size_t to_str(char *dst)
			{
				Genode::size_t offset = 0;

				for(int i = 0; i < count; i++)
				{
					dst[offset] = 46;
					Genode::memcpy(dst + 1 + offset, labels[i].addr, labels[i].len);
					offset += labels[i].len + 1;	
				}
				dst[offset] = 0;
				return offset + 1;

			}
		};
        
        class Resource_Record : public Genode::List<Resource_Record>::Element
        {
			
	        Genode::Heap 					*_heap;

			void * alloc(Genode::size_t s)
			{
				void *p = 0;
				_heap->alloc(s,&p);
				return p;
			}
			Name 				_name;
			//char*				_name;

        	Genode::int16_t		_type;
        	Genode::int16_t		__class;
        	Genode::int32_t		_ttl;
        	Genode::uint16_t	_length;
			
          public:
        	
        	char		        *_data;
            
			Genode::size_t str_len() { return _name.length(); }
        	Genode::uint16_t type() { return host_to_big_endian(_type); }
        	Genode::uint16_t _class() { return host_to_big_endian(__class); }
        	Genode::uint32_t ttl() { return host_to_big_endian(_ttl); }
        	Genode::uint16_t length() { return host_to_big_endian(_length); }
			Name* 			 name() { return &_name; }


        	void type(Genode::uint16_t v) { _type = host_to_big_endian((Genode::uint16_t)v); }
        	void _class(Genode::uint16_t v) { __class = host_to_big_endian((Genode::uint16_t)v); }
        	void ttl(Genode::uint32_t v) { _ttl = host_to_big_endian((Genode::uint32_t)v); }
        	void length(Genode::uint16_t v) { _length = host_to_big_endian((Genode::uint16_t)v); }

			Resource_Record(Genode::Heap *heap) : _heap(heap)
			{
				
				
			}

			Resource_Record(Genode::Heap *heap, const char* ptr) : _heap(heap), _name(ptr)
			{
				//_str_len = Genode::strlen(ptr) + 1;
				//_name = (char*) alloc(_str_len);
				
				Genode::memcpy(&_type, ptr + _name.length() , 10);
				_data = (char*) alloc(length());
				Genode::memcpy(_data, ptr + _name.length() + 10, length());
			}

			Resource_Record(Genode::Heap *heap, Name name, Ipv4_address ip) : _heap(heap), _name(name)
			{
				

				type(Type::A);
				_class(Class::CH);
				ttl(STANDARD_TTL);
				length(4);

				_data = (char*) alloc(length());
				Genode::memcpy(_data, &ip, length());
			}
			
			~Resource_Record()
			{
				_heap->free(_data, length());
				_heap->free(this, sizeof(Resource_Record));
			}

			Genode::size_t size()
			{
				//Genode::log("str_len: ", _str_len, " length() ", length(), " sum: ", _str_len +  10 + length());
				return _name.length() + (Genode::size_t) 10 + (Genode::size_t) length();
			}

        	Genode::size_t copy(char * dst)
        	{
        		Genode::size_t offset = Domain_Name_Ptr(_name.string()).copy(dst);
        		Genode::memcpy(dst + offset, &_type, 10);
        		Genode::memcpy(dst + offset + 10, _data, length());

				return offset + 10 + length();
        	}
			void *operator new(__SIZE_TYPE__ size, void *addr) { return addr; }
			
        };


	/*
        class RR_A : public Genode::List<RR_A>::Element
		{
		  private:
        	char		        *_name;
			Genode::size_t 		len;

          public:

			char*			 name()   { return _name; }
			Genode::uint16_t type()   { return host_to_big_endian(*(Genode::uint16_t*) (_name + len)); }
        	Genode::uint16_t Class()  { return host_to_big_endian(*(Genode::uint16_t*) (_name + len + 2)); }
        	Genode::uint32_t ttl()    { return host_to_big_endian(*(Genode::uint16_t*) (_name + len + 4)); }
        	Genode::uint16_t length() { return host_to_big_endian(*(Genode::uint16_t*) (_name + len + 8)); }
        	char*			 data()   { return _name + len + 10; }
        	
        	RR_A(char * ptr)
        	{
        		_name = ptr;
				len = Genode::strlen(ptr) + 1;
        	}

        	Genode::size_t size()
        	{
        		return 14 + len; 
        	}

			void copy(void *dst)
			{
				Genode::memcpy(dst, _name, len + 14);
			}

        };

		class RR_A_P : public Genode::List<RR_A_P>::Element
		{
			char 				*_name;
			Genode::uint16_t	_type;
        	Genode::uint16_t	__class;
        	Genode::uint32_t	_ttl;
        	Genode::uint16_t	_length;
        	char		        _data[4];

          public:
        	Genode::uint16_t type() { return host_to_big_endian(_type); }
        	Genode::uint16_t _class() { return host_to_big_endian(__class); }
        	Genode::uint32_t ttl() { return host_to_big_endian(_ttl); }
        	Genode::uint16_t length() { return host_to_big_endian(_length); }



        	void type(Genode::uint16_t v) { _type = host_to_big_endian((Genode::uint16_t)v); }
        	void _class(Genode::uint16_t v) { __class = host_to_big_endian((Genode::uint16_t)v); }
        	void ttl(Genode::uint32_t v) { _ttl = host_to_big_endian((Genode::uint32_t)v); }
        	void length(Genode::uint16_t v) { _length = host_to_big_endian((Genode::uint16_t)v); }

			RR_A_P(char *name, Ipv4_address ip)
        	{
        		ttl(STANDARD_TTL);
        		_class(Class::CH);
        		type(Type::A);
        		length(4);

				_name = name;
				Genode::memcpy(_data, &ip, 4);
        	}

			Genode::size_t copy(char *dst)
			{
				Domain_Name_Ptr dName(_name, 0);
				Genode::size_t offset = dName.copy(dst);
				Genode::memcpy(dst + offset, &_type, 14);

				return offset + 14;
			}

			Genode::size_t size()
			{
				return Genode::strlen(_name) + 2 + 14;
			}

			void *operator new(__SIZE_TYPE__ size, void *addr) { return addr; }

		};
	*/
        class Question : public Genode::List<Question>::Element
        {
	        Genode::Heap 					*_heap;

			void * alloc(Genode::size_t s)
			{
				void *p = 0;
				_heap->alloc(s,&p);
				return p;
			}

          public:
        	Name				_name;
        	Genode::uint16_t 	_type;
        	Genode::uint16_t 	_class;


			Question(Genode::Heap* heap , Name name, Genode::uint16_t __type, Genode::uint16_t __class)
			 : _heap(heap), _name(name), _type(host_to_big_endian(__type)), _class(host_to_big_endian(__class))
			{

			}

			Question(Genode::Heap* heap, const char * ptr)
			 : _heap(heap), _name(ptr), _type( *(ptr + _name.length())), _class( *(ptr + _name.length() + 2))
			{
				
			}

			~Question()
			{
				_heap->free(this, sizeof(Question));
			}

			Name* name() { return &_name; }

			//Question(char * ptr)
				//{
				//	parse(ptr);
			//}

			//void parse(char* ptr)
				//{
				//	
				//	Genode::size_t len = Genode::strlen(ptr);
				//	Genode::memcpy(_name   ,ptr          , len);
				//	Genode::memcpy(&_type  ,ptr + len + 1, 2);
				//	Genode::memcpy(&_class ,ptr + len + 3, 2);
				//	_class = host_to_big_endian(_class);
				//	_type = host_to_big_endian(_type);
				//
			//}

        	Genode::size_t size()
        	{	
				//with terminating zero byte, without List::Element
        		return 4 + _name.length() + 1; 
        	}

        	Genode::size_t copy(char * dst)
        	{
				Domain_Name_Ptr dName(_name.string());
        		Genode::size_t offset = dName.copy(dst);
				Genode::memcpy(dst + offset, &_type, 4);

				return offset + 4;
        	}

			void * operator new(__SIZE_TYPE__ size, void* addr) { return addr; }
        };

		class Question_Ptr //: public Genode::List<Question_Ptr>::Element
		{
			
		  protected:
			char 				*_name;
			Genode::uint16_t 	_len;
			Genode::uint16_t 	*_type;
			Genode::uint16_t 	*_class;
		  	  
		  public: 
			Question_Ptr(char * ptr)
			{
				_len = Genode::strlen(ptr) + 1;
				_name = ptr;
				
			}

			const char*		 name()   { return _name; }
			Genode::uint16_t type()   { return host_to_big_endian(*(Genode::uint16_t*) (_name + _len)); }
        	Genode::uint16_t Class()  { return host_to_big_endian(*(Genode::uint16_t*) (_name + _len + 2)); }

			void copy(char* dst)
			{
				Genode::memcpy(dst, _name, _len + 4);
			}

			void log()
			{
				Genode::log((const char*)_name);
			}

			Genode::size_t length()
			{ return _len; }
			Genode::size_t size()
			{ return _len + 4;}

			//void * operator new(__SIZE_TYPE__ size, void* addr) { return addr; }
		};

    };
}

#endif
