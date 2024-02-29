/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file file.h
 * @author Danh Pham
 * @date 10 Nov 2020
 * @version: draft 2.0.0
 * @brief Template for doxygen automation document generate.
 * Add more detail desciption here
 */
 
 #ifndef _TEMPLATE_H_
 #define _TEMPLATE_H_
 
 #include <stdint.h>
 #include "header_template.h"

/*!
*	@def MAX(x, y)
* Computes maximum of @a x and @a y
*/
/*!
*	@def ABS(x)
* Computes absolute value of @a x
*/
#define MAX(x, y)		(((x)>(y))?(x):(y))
#define ABS(x)			(((x) > 0)?(x):(-(x)))
 
/*! @struct STRU_DEMO
* A description of the struct.
*/
typedef struct
{
	uint8_t u8_demo_value;	/**< Some documentation for u8_demo_value */
}STRU_DEMO;

/*! @enum E_DEMO
* A description of the struct.
*/
typedef enum
{
	DEMO_VALUE = 0,  /**< Some documentation for first. */
	DEMO_VALUE2 = 1, /**< Some documentation for sencond. */
}E_DEMO;

/*!
*	Extern functions
*/
extern void v_function_demo(uint8_t u8_var1, uint32_t u32_var2);
#endif /* _TEMPLATE_H_ */

