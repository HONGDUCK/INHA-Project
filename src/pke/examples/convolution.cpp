//==================================================================================
// BSD 2-Clause License
//
// Copyright (c) 2014-2022, NJIT, Duality Technologies Inc. and other contributors
//
// All rights reserved.
//
// Author TPOC: contact@openfhe.org
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//==================================================================================

/**
 * Simple example for BFV and CKKS for inner product.
 */

#include <iostream>
#include "openfhe.h"
#include <vector>
#include <cmath>

using namespace lbcrypto;

template <class T>
T plainInnerProduct(std::vector<T> vec) {
    T res = 0.0;
    for (auto& el : vec) {
        res += (el * el);
    }
    return res;
}

bool innerProductBFV(std::vector<int64_t>& incomingVector) {
    int64_t expectedResult = plainInnerProduct(incomingVector);
    /////////////////////////////////////////////////////////
    // Crypto CryptoParams
    /////////////////////////////////////////////////////////
    CCParams<CryptoContextBFVRNS> parameters;
    parameters.SetPlaintextModulus(65537);
    parameters.SetMultiplicativeDepth(20);
    parameters.SetSecurityLevel(lbcrypto::HEStd_NotSet);
    parameters.SetRingDim(1 << 7);
    uint32_t batchSize = parameters.GetRingDim() / 2;

    /////////////////////////////////////////////////////////
    // Set crypto params and create context
    /////////////////////////////////////////////////////////
    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc;
    cc = GenCryptoContext(parameters);

    // Enable the features that you wish to use.
    cc->Enable(PKE);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);

    KeyPair keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    cc->EvalSumKeyGen(keys.secretKey);

    Plaintext plaintext1 = cc->MakePackedPlaintext(incomingVector);
    auto ct1             = cc->Encrypt(keys.publicKey, plaintext1);
    auto finalResult     = cc->EvalInnerProduct(ct1, ct1, batchSize);
    lbcrypto::Plaintext res;
    cc->Decrypt(keys.secretKey, finalResult, &res);
    auto final = res->GetPackedValue()[0];

    std::cout << "Expected Result: " << expectedResult << " Inner Product Result: " << final << std::endl;
    return expectedResult == final;
}

void Convolution_in_CKKS(const std::vector<double>& incomingVector, const std::vector<double>& kernel, int st,int inputsize, int row,int kernelrow,int convcount) {
 
    lbcrypto::SecurityLevel securityLevel = lbcrypto::HEStd_NotSet;
    uint32_t dcrtBits                     = 59;
    uint32_t ringDim                      = 1 << 8;
    uint32_t batchSize                    = ringDim / 2;
    lbcrypto::CCParams<lbcrypto::CryptoContextCKKSRNS> parameters;
    uint32_t multDepth = 10;

    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(dcrtBits);
    parameters.SetBatchSize(batchSize);
    parameters.SetSecurityLevel(securityLevel);
    parameters.SetRingDim(ringDim);

    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc;
    cc = GenCryptoContext(parameters);

    cc->Enable(PKE);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);

    KeyPair keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    cc->EvalSumKeyGen(keys.secretKey);
    std::vector<int32_t> indexList = {-st}; //stride 1
    cc->EvalRotateKeyGen(keys.secretKey, indexList);

    std::vector<double> conv_result(inputsize, 0.0);
    std::vector <double > init;
    for(int i=0; i<inputsize; i++)
    {
        if(i ==0){
            init.push_back(1.0);
        }
        else{
            init.push_back(0.0);
        }
    }
    
    Plaintext plaintext1 = cc->MakeCKKSPackedPlaintext(incomingVector);
    Plaintext plaintext2 = cc->MakeCKKSPackedPlaintext(kernel);
    Plaintext plaintext3 = cc->MakeCKKSPackedPlaintext(conv_result);
    Plaintext plaintext4 = cc->MakeCKKSPackedPlaintext(init);

    auto ct1             = cc->Encrypt(keys.publicKey, plaintext1);
    auto ct2             = cc->Encrypt(keys.publicKey, plaintext2);
    auto convol_result     = cc->Encrypt(keys.publicKey, plaintext3);
    auto init1           = cc->Encrypt(keys.publicKey, plaintext4);

    int count =0;
    
     while (count != convcount){
        
        if(count%row <= row - kernelrow){
            auto inp1 =     cc->EvalInnerProduct(ct1,ct2,batchSize);
            auto temp = cc->EvalMult(init1, inp1); //[1,0,0,0, ...,0]* inp1
            auto temp1 = cc->EvalAdd(temp, convol_result); // [temp, 0, 0, 0, 0, ... ,0 ] 
            auto rot2 = cc->EvalRotate(init1,indexList[0]); 
            init1 = rot2; // [0, 1, 0, 0, 0 , ... , 0]
            convol_result = temp1;
        }
        count++;
        auto rot1 = cc->EvalRotate(ct2, indexList[0]); //kernel rotation
        ct2 = rot1;
    }
    
    lbcrypto::Plaintext res;
    cc->Decrypt(keys.secretKey, convol_result, &res);
    res->SetLength(incomingVector.size());
    std::cout  << " Convolution Result: " << *res << std::endl;
    
}
 
int main(int argc, char* argv[]) {
    std::vector<int64_t> vec = {1, 2, 3, 4, 5, 10, 14, 2, 3, 5, 6, 4, 3, 2, 3, 4, 5, 2, 6, 7, 11, 2, 12, 5, 5};
    // 5*5 matrix
    //{1, 2, 3, 4, 5,
    // 10, 14, 2, 3, 5,
    // 6, 4, 3, 2, 3, 
    // 4, 5, 2, 6, 7,
    // 11, 2, 12, 5, 5}

    std::vector<int64_t> kernel = {1, -1, 1, 0, 0, 0, 1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //3*3 matrix
    //{1, -1, 0,
    // 0, 1, 1, 
    // -1, 0, 1}

    std::vector<double> inputDouble(vec.begin(), vec.end());
    std::vector<double> kernelDouble(kernel.begin(),kernel.end());
    int st = 1;
    int inputsize = vec.size();
    int row = sqrt(inputsize);
    int kernelsize = 9; //kernel size 변수로 잡는 법을 잘 모르겠음.
    int kernelrow = sqrt(kernelsize);
    int convcount = row*kernelrow;

    Convolution_in_CKKS(inputDouble, kernelDouble, st,inputsize, row, kernelrow, convcount);

    return 0;
}
