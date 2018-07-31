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
#include <dlfcn.h>
#include <sstream>

#include "amazon/dsstne/engine/GpuTypes.h"
#include "amazon/dsstne/engine/NNTypes.h"
#include "amazon/dsstne/engine/NNLayer.h"

#include "jni_util.h"
#include "com_amazon_dsstne_Dsstne.h"

using namespace dsstne;

namespace {
const unsigned long SEED = 12134ull;

void *LIB_MPI = NULL;
const char* LIB_MPI_SO = "libmpi.so";

const int ARGC = 1;
char *ARGV = "jni-faux-process";

jni::References REFS;

jmethodID java_ArrayList_;
jmethodID java_ArrayList_add;

jmethodID dsstne_NNLayer_;

GpuContext* checkPtr(JNIEnv *env, jlong ptr) {
  GpuContext *gpuContext = (GpuContext*) ptr;
  if (gpuContext == NULL) {
    std::stringstream msg;
    msg << "GpuContext pointer is null, call init prior to any other functions";
    jni::throwJavaException(env, jni::RuntimeException, msg.str());
  }
  return gpuContext;
}
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  /*
   * JVM loads dynamic libs into a local namespace,
   * MPI requires to be loaded into a global namespace
   * so we manually load it into a global namespace here
   */
  LIB_MPI = dlopen(LIB_MPI_SO, RTLD_NOW | RTLD_GLOBAL);

  if (LIB_MPI == NULL) {
    fprintf(stderr, "Failed to load libmpi.so\n");
    exit(1);
  }

  JNIEnv* env;
  if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  } else {
//    jni::findClassGlobalRef(env, REFS, jni::NNLayer);
//    jni::findClassGlobalRef(env, REFS, jni::ArrayList);

    java_ArrayList_ = jni::findConstructorId(env, REFS, jni::ArrayList, jni::NO_ARGS_CONSTRUCTOR);
    java_ArrayList_add = jni::findMethodId(env, REFS, jni::ArrayList, "add", "(Ljava/lang/Object;)Z");

    dsstne_NNLayer_ = jni::findConstructorId(env, REFS, jni::NNLayer, "(Ljava/lang/String;IIIIII)V");

    return JNI_VERSION_1_6;
  }
}

void JNI_OnUnload(JavaVM *vm, void *reserved) {
  using namespace dsstne;

  JNIEnv* env;
  if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
    return;
  } else {
    jni::deleteReferences(env, REFS);
  }
}

JNIEXPORT jlong JNICALL Java_com_amazon_dsstne_Dsstne_load(JNIEnv *env, jclass clazz, jstring jNetworkFileName,
                                                           jint batchSize) {
  const char *networkFileName = env->GetStringUTFChars(jNetworkFileName, 0);

  getGpu().Startup(ARGC, &ARGV);
  getGpu().SetRandomSeed(SEED);
  NNNetwork *network = LoadNeuralNetworkNetCDF(networkFileName, batchSize);
  getGpu().SetNeuralNetwork(network);

  env->ReleaseStringUTFChars(jNetworkFileName, networkFileName);
  return (jlong) &getGpu();
}

JNIEXPORT void JNICALL Java_com_amazon_dsstne_Dsstne_shutdown(JNIEnv *env, jclass clazz, jlong ptr) {
  GpuContext *gpuContext = checkPtr(env, ptr);
  gpuContext->Shutdown();
}

JNIEXPORT jobject JNICALL Java_com_amazon_dsstne_Dsstne_get_1layers(JNIEnv *env, jclass clazz, jlong ptr,
                                                                    jint kindOrdinal) {
  GpuContext *gpuContext = checkPtr(env, ptr);
  NNNetwork *network = gpuContext->_pNetwork;
  NNLayer::Kind kind = static_cast<NNLayer::Kind>(kindOrdinal);

  std::vector<const NNLayer*> layers;
  std::vector<const NNLayer*>::iterator it = network->GetLayers(kind, layers);
  if (it == layers.end()) {
    std::stringstream msg;
    msg << "No layers of type " << NNLayer::_sKindMap[kind] << " found in network: " << network->GetName();
    jni::throwJavaException(env, jni::RuntimeException, msg.str());
  }

  jobject jLayers = jni::newObject(env, REFS, jni::ArrayList, java_ArrayList_);

  for (; it != layers.end(); ++it) {
    const NNLayer* layer = *it;
    const std::string name = layer->GetName();
    int kind = static_cast<int>(layer->GetKind());
    uint32_t attributes = layer->GetAttributes();
    jstring jName = env->NewStringUTF(name.c_str());
    uint32_t numDim = layer->GetNumDimensions();

    uint32_t lx, ly, lz, lw;
    std::tie(lx, ly, lz, lw) = layer->GetDimensions();

    jobject jInputLayer = jni::newObject(env, REFS, jni::NNLayer, dsstne_NNLayer_, jName, kind, attributes, numDim, lx,
                                         ly, lz);

    jclass java_util_ArrayList = REFS.getClassGlobalRef(jni::ArrayList);
    env->CallBooleanMethod(jLayers, java_ArrayList_add, jInputLayer);
  }
  return jLayers;
}

JNIEXPORT void JNICALL Java_com_amazon_dsstne_Dsstne_predict(JNIEnv *env, jclass clazz, jlong ptr, jint k,
                                                             jobjectArray jInputs, jobjectArray jOutputIndexes,
                                                             jobjectArray jOutputScores) {
  GpuContext *gpuContext = checkPtr(env, ptr);
  NNNetwork *network = gpuContext->_pNetwork;

  jsize len = env->GetArrayLength(jobjectArray);

  for (jsize i = 0; i < len; ++i) {
      jobject jInputNNDataSet = env->GetObjectArrayElement(jInputs, i);

  }
}

