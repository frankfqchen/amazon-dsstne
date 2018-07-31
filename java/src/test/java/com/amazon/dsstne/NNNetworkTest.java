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

package com.amazon.dsstne;

import java.io.IOException;

import org.junit.Test;

public class NNNetworkTest {

  @Test
  public void test() throws IOException {
    NetworkConfig config = NetworkConfig.with().networkFilePath("/home/kiuk/tmp/gl.nc").build();

    try(NNNetwork network = Dsstne.load(config)) {
      for (NNLayer layer : network.getInputLayers()) {
        System.out.println("Found input layer: " + layer);
      }

      for (NNLayer layer : network.getOutputLayers()) {
        System.out.println("Found output layer: " + layer);
      }
    }
  }
}
