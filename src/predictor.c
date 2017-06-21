//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
//#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Mayu Zhang";
const char *studentID   = "A53205212";
const char *email       = "maz030@eng.ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
uint8_t *gsharebht;   //gshare branch histrory table
uint32_t ghr;       //global history register except CUSTOM
uint32_t gsharebhtentries;    //number of entries in gsharebht
uint32_t ghmask;   //ghr mask, shared by all predictors except CUSTOM

uint8_t *lbht;      //local branch history table
uint8_t *global;     //global branch history table
uint8_t *chooser;    //chooser in TOURNAMENT predictor
uint32_t *lpht;      //local pattern history table
uint32_t lbhtentries;     //number of entries in lbht
uint32_t globalentries;    //number of entries in global bht
uint32_t chooserentries;   //number of entries in chooser
uint32_t lphtentries;      //number of entries in lpht
uint32_t pcmask;
uint32_t lphtmask;

// CUSTOM implements piecewise linear predictor introduced in following papers:
// Idealized Piecewise Linear Branch Prediction, Daniel A. Jimenez
// Piecewise Linear Branch Prediction, Daniel A. Jimenez

//#define n 16
//#define m 16
//#define GHL 16
//int W[n][m][GHL];    //8 bits used actually, due to the threshold is 128, so memory is 16*16*16*8 = 64k bits
//double theta = 2.14 * GHL + 20.58;
//uint8_t GA[GHL];     //4 bits used actually, memory is 4*16 = 64 bits

#define GHL 48       //all the following parameters and values are given by paper
#define theta 100
#define H1 511387
#define H2 660509
#define H3 1289381

uint64_t ghrcustom;   //global history register for CUSTOM
uint64_t ghmaskcustom; //ghr mask for CUSTOM
int W[8590];  //7 bits used, due to the limit is 64
int GA[GHL];  //14 bits used, due to the limit is 8589
int output;
uint8_t predict;
uint32_t bitmask;
uint32_t addr;
uint32_t tmpaddr;

// memory calculation for W and GA arrays:
// 8590 * 7 + 48 * 14 = 60802 bits

//


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
    switch(bpType){
        case GSHARE:
            ghr = 0;
            ghmask = (1 << ghistoryBits) - 1;
            gsharebhtentries = 1 << ghistoryBits;
            gsharebht = (uint8_t *) malloc(sizeof(uint8_t) * gsharebhtentries);
            for(uint32_t i = 0; i < gsharebhtentries; i++){
                gsharebht[i] = WN;
            }
            break;
        case TOURNAMENT:
            ghr = 0;
            pcmask = (1 << pcIndexBits) - 1;
            lphtmask = (1 << lhistoryBits) - 1;
            ghmask = (1 << ghistoryBits) - 1;
            lbhtentries = 1 << lhistoryBits;
            lphtentries = 1 << pcIndexBits;
            globalentries = 1 << ghistoryBits;
            chooserentries = 1 << ghistoryBits;
            lbht = (uint8_t *)malloc(sizeof(uint8_t) * lbhtentries);
            lpht = (uint32_t *)malloc(sizeof(uint32_t) * lphtentries);
            global = (uint8_t *)malloc(sizeof(uint8_t) * globalentries);
            chooser = (uint8_t *)malloc(sizeof(uint8_t) * chooserentries);
            for(uint32_t i = 0; i < lbhtentries; i++)
                lbht[i] = WN;
            for(uint32_t i = 0; i < globalentries; i++)
                global[i] = WN;
            for(uint32_t i = 0; i < chooserentries; i++)
                chooser[i] = WT;
            for(uint32_t i = 0; i < lphtentries; i++)
                lpht[i] = 0;
            break;
        case CUSTOM:
//            ghr = 0;
//            for(int i = 0; i < n; i++){
//                for(int j = 0; j < m; j++){
//                    for(int k = 0; k < GHL; k++)
//                        W[i][j][k] = 0;
//                }
//            }
            ghrcustom = 0;
            for(int i = 0; i < 8590; i++)
                W[i] = 0;
            for(int i = 0; i < GHL; i++)
                GA[i] = 0x00;
            bitmask = 1;
            predict = 0;
            ghmaskcustom = ((uint64_t)1 << GHL) - 1;
            break;
        default:
            break;
    }

  //
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
    uint8_t result = 0;
    uint32_t xor;
    uint8_t choose;

    uint32_t temp;
    
    
    switch (bpType) {
        case GSHARE:
            xor = (pc & ghmask) ^ ghr;
            result = gsharebht[xor];
            break;
        case TOURNAMENT:
            choose = chooser[ghr];
            if(choose >> 1)
                result = global[ghr];
            else{
                result = lbht[lpht[pc & pcmask]];
            }
            break;
        case CUSTOM:
//            addr = pc % n;
//            output = W[addr][0][0];
//            for(int i = 0; i < GHL; i++){
//                temp = (ghr & bitmask) >> i;
//                if(temp == 1)
//                    output += W[addr][GA[i]][i];
//                else
//                    output -= W[addr][GA[i]][i];
//                bitmask = bitmask << 1;
//            }
//            bitmask = 1;
//            predict = (output >= 0) ? 1: 0;
            addr = (pc * H1) % 8590;
            output = W[addr];
            for(int i = 0; i < GHL; i++){
                temp = (ghrcustom & bitmask) >> i;
                tmpaddr = ((pc * H1) ^ (GA[i] * H2) ^ (i * H3)) % 8590;
                if(temp == 1){
                    output += W[tmpaddr];
                }
                else
                    output -= W[tmpaddr];
                bitmask = bitmask << 1;
            }
            bitmask = 1;
            predict = (output >= 3) ? 1: 0;
            break;
        default:
            break;
    }
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
          return result >> 1;
    case TOURNAMENT:
          return result >> 1;
    case CUSTOM:
          return predict;
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
    uint32_t xor;
    uint32_t pattern;
    uint32_t tmp;
    switch (bpType) {
        case GSHARE:
            xor = (pc & ghmask) ^ ghr;
            if(outcome == 1 && gsharebht[xor] != ST){
                gsharebht[xor]++;
            }
            if(outcome == 0 && gsharebht[xor] != SN){
                gsharebht[xor]--;
            }
            ghr = ((ghr << 1) | outcome) & ghmask;
            break;
        case TOURNAMENT:
            pattern = lpht[pc & pcmask];
            if(lbht[pattern] >> 1 == outcome && global[ghr] >> 1 != outcome){
                if(chooser[ghr] != SN)
                    chooser[ghr]--;
            }
            else if(global[ghr] >> 1 == outcome && lbht[pattern] >> 1 != outcome){
                if(chooser[ghr] != ST)
                    chooser[ghr]++;
            }
            if(outcome == 1 && lbht[pattern] != ST)
                lbht[pattern]++;
            if(outcome == 1 && global[ghr] != ST)
                global[ghr]++;
            if(outcome == 0 && lbht[pattern] != SN)
                lbht[pattern]--;
            if(outcome == 0 && global[ghr] != SN)
                global[ghr]--;
            
            lpht[pc & pcmask] = ((pattern << 1) | outcome) & lphtmask;
            ghr = ((ghr << 1) | outcome) & ghmask;
            break;
        case CUSTOM:
//            if((output > ((-1) * theta) && output < theta) || predict != outcome){
//                if(outcome == 1){
//                    if(W[addr][0][0] < 128)
//                        W[addr][0][0]++;
//                }
//                else{
//                    if(W[addr][0][0] >= -128)
//                        W[addr][0][0]--;
//                }
//                for(int i = 0; i < GHL; i++){
//                    tmp = (ghr & bitmask) >> i;
//                    if(tmp == outcome){
//                        if(W[addr][0][0] < 128)
//                            W[addr][GA[i]][i]++;
//                    }
//                    else{
//                        if(W[addr][0][0] >= -128)
//                            W[addr][GA[i]][i]--;
//                    }
//                    bitmask = bitmask << 1;
//                }
//                bitmask = 1;
//            }
//            for(int i = GHL-1; i >= 1; i--){
//                GA[i] = GA[i-1];
//            }
//            GA[0] = addr;
//            ghr = ((ghr << 1) | outcome) & ghmask;
            if((output > ((-1) * theta) && output < theta) || predict != outcome){
                if(outcome == 1){
                    if(W[addr] < 64)
                        W[addr]++;
                }
                else{
                    if(W[addr] >= -64)
                        W[addr]--;
                }
                for(int i = 0; i < GHL; i++){
                    tmp = (ghrcustom & bitmask) >> i;
                    tmpaddr = ((pc * H1) ^ (GA[i] * H2) ^ (i * H3)) % 8590;
                    if(tmp == outcome){
                        if(W[tmpaddr] < 64)
                            W[tmpaddr]++;
                    }
                    else{
                        if(W[tmpaddr] >= -64)
                            W[tmpaddr]--;
                    }
                    bitmask = bitmask << 1;
                }
                bitmask = 1;
            }
            for(int i = GHL-1; i >= 1; i--){
                GA[i] = GA[i-1];
            }
            GA[0] = addr;
            ghrcustom = ((ghrcustom << 1) | outcome) & ghmaskcustom;
            break;
        default:
            break;
    }
  //
}

void memclean(){
    free(gsharebht);
    free(lbht);
    free(global);
    free(lpht);
    free(chooser);
}
