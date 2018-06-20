/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
#include "madara/MadaraExport.h"
/* Header for class ai_madara_logger_GlobalLogger */

#ifndef _Included_ai_madara_logger_GlobalLogger
#define _Included_ai_madara_logger_GlobalLogger
#ifdef __cplusplus
extern "C" {
#endif
/*
* Class:     ai_madara_logger_GlobalLogger
* Method:    jni_getCPtr
* Signature: ()J
*/
MADARA_EXPORT jlong JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1getCPtr
  (JNIEnv *, jclass);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_setLevel
 * Signature: (I)V
 */
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1setLevel
  (JNIEnv *, jclass, jint);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_getLevel
 * Signature: ()I
 */
MADARA_EXPORT jint JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1getLevel
  (JNIEnv *, jclass);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_getTag
 * Signature: ()Ljava/lang/String;
 */
MADARA_EXPORT jstring JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1getTag
  (JNIEnv *, jclass);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_setTag
 * Signature: (Ljava/lang/String;)V
 */
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1setTag
  (JNIEnv *, jclass, jstring);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_addTerm
 * Signature: ()V
 */
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1addTerm
  (JNIEnv *, jclass);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_addSyslog
 * Signature: ()V
 */
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1addSyslog
  (JNIEnv *, jclass);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_clear
 * Signature: ()V
 */
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1clear
  (JNIEnv *, jclass);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_addFile
 * Signature: (Ljava/lang/String;)V
 */
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1addFile
  (JNIEnv *, jclass, jstring);

/*
 * Class:     ai_madara_logger_GlobalLogger
 * Method:    jni_log
 * Signature: (Ljava/lang/String;)V
 */
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1log
(JNIEnv *, jclass, jint, jstring);

/*
* Class:     ai_madara_logger_GlobalLogger
* Method:    jni_setTimestampFormat
* Signature: (Ljava/lang/String;)V
*/
MADARA_EXPORT void JNICALL
Java_ai_madara_logger_GlobalLogger_jni_1setTimestampFormat
(JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif
#endif