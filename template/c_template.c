/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file file.c
 * @author Danh Pham
 * @date 10 Nov 2020
 * @version: draft 2.0.0
 * @brief Template for doxygen automation document generate.
 * Add more detail desciption here
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>

/*!
*	Variables declare
*/
static uint8_t u8_demo_var; /**< desciption for u8_demo_var */
/*!
*	Private functions prototype
*/
static uint8_t u8_static_demo (uint8_t u8_var1, uint8_t u8_var2, char *c_text);
/*!
*	Public functions
*/
/*!
*	@fn v_function_demo(uint8_t u8_var1, uint32_t u32_var2)
* @brief Description of function
* @param[in] u8_var1 description for u8_var1
* @param[in] u32_var2 description for u32_var2
* @return None
*/
void v_function_demo (uint8_t u8_var1, uint32_t u32_var2)
{
	//Do some thing without return value.
}

/*!
*	Private functions
*/
/*!
*	@fn u8_static_demo(uint8_t u8_var1, uint8_t u8_var2, char *c_text)
* @brief Description of function
* @param[in] u8_var1 description for u8_var1
* @param[in] u8_var2 description for u8_var2
* @param[out] c_text description for c_text
* @return description for return value
*/
static uint8_t u8_static_demo(uint8_t u8_var1, uint8_t u8_var2, char *c_text)
{
	// Do some thing
	return 0;
}
