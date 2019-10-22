

#ifndef __GW_MACROS_UTIL_H
#define __GW_MACROS_UTIL_H

#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
 
/* include/linux/kernel.h:
 * container_of - cast a member of a structure out to the containing structure
 * @ptr: the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:    the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({	    \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offset_of(type,member) );})





#endif /* __GW_MACROS_UTIL_H */
