/*
 *  Copyright 2016  Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License").
 *  You may not use this file except in compliance with the License.
 *  A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0/
 *
 *  or in the "license" file accompanying this file.
 *  This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 *  either express or implied.
 *
 *  See the License for the specific language governing permissions and limitations under the License.
 *
 */
#include <map>
#include <iostream>
#include <string>
#include <sstream>

#include "jni_util.h"

namespace {
const std::string CONSTRUCTOR_METHOD_NAME = "<init>";
}  // namespace

namespace dsstne {
namespace jni {

/* exceptions */
const std::string RuntimeException = "java/lang/RuntimeException";
const std::string NullPointerException = "java/lang/NullPointerException";
const std::string IllegalStateException = "java/lang/IllegalStateException";
const std::string IllegalArgumentException = "java/lang/IllegalArgumentException";
const std::string ClassNotFoundException = "java/lang/ClassNotFoundException";
const std::string NoSuchMethodException = "java/lang/NoSuchMethodException";
const std::string FileNotFoundException = "java/io/FileNotFoundException";
const std::string UnsupportedOperationException = "java/lang/UnsupportedOperationException";

/* collections */
const std::string ArrayList = "java/util/ArrayList";

/* java types */
const std::string String = "java/lang/String";

/* custom types */
const std::string NNLayer = "com/amazon/dsstne/NNLayer";
const std::string NNDataSet = "com/amazon/dsstne/NNDataSet";
const std::string OutputNNDataSet = "com/amazon/dsstne/data/OutputNNDataSet";

/* methods */
const std::string NO_ARGS_CONSTRUCTOR = "()V";

void deleteReferences(JNIEnv *env, References &refs) {
  for (auto &entry : refs.classGlobalRefs) {
    env->DeleteGlobalRef(entry.second);
  }
  refs.classGlobalRefs.clear();
}

jclass References::getClassGlobalRef(const std::string &className) const {
  return classGlobalRefs.at(className);
}

bool References::containsClassGlobalRef(const std::string &className) const {
  return classGlobalRefs.find(className) != classGlobalRefs.end();
}

void References::putClassGlobalRef(const std::string &className, jclass classRef) {
  classGlobalRefs[className] = classRef;
}

void throwJavaException(JNIEnv *env, std::string &exceptionType, std::string &msg) {
  std::string message = msg;
  jclass exc = env->FindClass(exceptionType.c_str());
  if (exc == 0) {
    // throw a ClassNotFoundException if the requested exceptionType does not exist
    exc = env->FindClass(jni::ClassNotFoundException.c_str());

    std::stringstream m;
    m << "Cannot throw " << exceptionType << ":" << msg << "." << exceptionType << " does not exist.";
    message = m.str();
  }

  env->ThrowNew(exc, message.c_str());
}

void throwJavaException(JNIEnv *env, const std::string &exceptionType, const std::string &msg) {
  throwJavaException(env, exceptionType, msg.c_str());
}

/**
 * Finds the provided class by name and adds a global reference to it to References. Subsequent findMethodId
 * calls on the same class do not have to be added as a global reference since the global reference
 * to the class keeps the class from being unloaded and hence also the method/field references.
 * Once done with the class reference, the global reference must be explicitly deleted to prevent
 * memory leaks.
 */
jclass findClassGlobalRef(JNIEnv *env, References &refs, const std::string &className) {
  if(refs.containsClassGlobalRef(className)) {
    return refs.getClassGlobalRef(className);
  }

  // this returns a local ref, need to create a global ref from this
  jclass classLocalRef = env->FindClass(className.c_str());
  if (classLocalRef == NULL) {
    throwJavaException(env, jni::ClassNotFoundException, className);
  }

  jclass classGlobalRef = (jclass) env->NewGlobalRef(classLocalRef);
  refs.putClassGlobalRef(className, classGlobalRef);
  env->DeleteLocalRef(classLocalRef);
  return classGlobalRef;
}

jmethodID findMethodId(JNIEnv *env, References &refs, const std::string &className, const std::string &methodName,
                  const std::string &methodDescriptor) {
  jclass clazz = findClassGlobalRef(env, refs, className);
  jmethodID methodId = env->GetMethodID(clazz, methodName.c_str(), methodDescriptor.c_str());

  if (methodId == NULL) {
    std::stringstream msg;
    msg << className << "#" << methodName << methodDescriptor;
    throwJavaException(env, jni::NoSuchMethodException, msg.str());
  }

  return methodId;
}

jmethodID findConstructorId(JNIEnv *env, References &refs, const std::string &className,
                       const std::string &methodDescriptor) {
  return findMethodId(env, refs, className, CONSTRUCTOR_METHOD_NAME, methodDescriptor);
}

jobject newObject(JNIEnv *env, const References &refs, const std::string &className,
                  jmethodID jConstructor, ...) {

  jclass clazz = refs.getClassGlobalRef(className);

  va_list args;
  va_start(args, jConstructor);
  jobject obj = env->NewObjectV(clazz, jConstructor, args);
  va_end(args);

  if (obj == NULL) {
    std::stringstream msg;
    msg << "Unable to create new object: " << className << "#" << CONSTRUCTOR_METHOD_NAME;
    throwJavaException(env, jni::RuntimeException, msg.str());
  }
  return obj;
}
}  // namespace jni
}  // namsspace dsstne
