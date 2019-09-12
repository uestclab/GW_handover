

#ifndef __MACROS_UTIL_H
#define __MACROS_UTIL_H

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





#endif /* __MACROS_UTIL_H */

/* example */

// struct student {
// 	char name[32];
// 	int age;
// 	double score;
// };

// struct student *temp_jack;
// struct student jack = {"jack", 18, 89.4};
// int *page = &(jack.age); 	
// temp_jack = container_of(page, struct student, age);
// printf("jack's name is %s\n",temp_jack->name);
// printf("jack's age is %d\n",temp_jack->age);
// printf("jack's score is %.2f\n",temp_jack->score);