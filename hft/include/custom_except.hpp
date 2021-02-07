/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

//
// Macro appropriable for simple
// definition of custom exception.
//

#ifndef DEFINE_CUSTOM_EXCEPTION_CLASS

#define DEFINE_CUSTOM_EXCEPTION_CLASS(__EXCEPT_CLASS_NAME__, __ANCESTOR_CLASS__) \
    class __EXCEPT_CLASS_NAME__ : public __ANCESTOR_CLASS__ \
    { \
    public: \
        __EXCEPT_CLASS_NAME__(const std::string &err_msg) \
           : __ANCESTOR_CLASS__(err_msg) {} \
    };

#endif /* DEFINE_CUSTOM_EXCEPTION_CLASS */
