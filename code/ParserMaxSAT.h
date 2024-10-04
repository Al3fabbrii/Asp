/*!
 * \author Ruben Martins - ruben@sat.inesc-id.pt
 *
 * @section LICENSE
 *
 * MiniSat,  Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 *           Copyright (c) 2007-2010, Niklas Sorensson
 * Open-WBO, Copyright (c) 2013-2015, Ruben Martins, Vasco Manquinho, Ines Lynce
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef ParserMaxSAT_h
#define ParserMaxSAT_h

#include <stdio.h>
#include <map>
#include <string>

#include "MaxSATFormula.h"
#include "core/SolverTypes.h"
#include "utils/ParseUtils.h"


using NSPACE::mkLit;
using NSPACE::StreamBuffer;

std::map<int,std::string> aspifShow;
namespace openwbo
{
  
  //=================================================================================================
  // DIMACS Parser:
  

  template <class B>
  static uint64_t parseWeight(B &in)
  {
    uint64_t val = 0;
    while ((*in >= 9 && *in <= 13) || *in == 32)
      ++in;
    // MSE'20 Format Support
    if (*in == 'h')
    {
      ++in;
      return UINT64_MAX;
    }
    if (*in < '0' || *in > '9')
      fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    while (*in >= '0' && *in <= '9')
      val = val * 10 + (*in - '0'), ++in;

    return val;
  }

  template <class B, class MaxSATFormula>
  static uint64_t readClause(B &in, MaxSATFormula *maxsat_formula,
                             vec<Lit> &lits)
  {
    int parsed_lit, var;
    uint64_t weight = 1;
    lits.clear();
    if (maxsat_formula->getProblemType() == _WEIGHTED_)
      weight = parseWeight(in);

    for (;;)
    {
      parsed_lit = parseInt(in);
      if (parsed_lit == 0)
        break;
      var = abs(parsed_lit) - 1;
      while (var >= maxsat_formula->nVars())
        maxsat_formula->newVar();
      lits.push((parsed_lit > 0) ? mkLit(var) : ~mkLit(var));
    }
    return weight;
  }

  template <class B, class MaxSATFormula>
  static void parseMaxSAT(B &in, MaxSATFormula *maxsat_formula)
  {
    vec<Lit> lits;
    uint64_t hard_weight = UINT64_MAX;
    // int count = 0;

    for (;;)
    {
      skipWhitespace(in);
      if (*in == EOF)
        break;
      else if (*in == 'p')
      {
        if (eagerMatch(in, "p cnf"))
        {
          parseInt(in); // Variables
          parseInt(in); // Clauses
        }
        else if (eagerMatch(in, "wcnf"))
        {
          parseInt(in); // Variables
          parseInt(in); // Clauses
          if (*in != '\r' && *in != '\n')
          {
            hard_weight = parseWeight(in);
            maxsat_formula->setHardWeight(hard_weight);
          }
        }
        else
          printf("c PARSE ERROR! Unexpected char: %c\n", *in),
              printf("s UNKNOWN\n"), exit(_ERROR_);
      }
      else if (*in == 'c' || *in == 'p')
        skipLine(in);
      else
      {
        uint64_t weight = readClause(in, maxsat_formula, lits);
        if (weight != hard_weight ||
            maxsat_formula->getProblemType() == _UNWEIGHTED_)
        {
          if (weight == 0)
          {
            printf("c WARNING: a 0-weighted clause is ignored\n");
            continue;
          }
          // Updates the maximum weight of soft clauses.
          maxsat_formula->setMaximumWeight(weight);
          // Updates the sum of the weights of soft clauses.
          maxsat_formula->updateSumWeights(weight);
          //        printf("Adding soft clause : %d\n",++count);
          maxsat_formula->addSoftClause(weight, lits);
        }
        else
          maxsat_formula->addHardClause(lits);
      }
    }
  }

  // Inserts problem into solver.
  //

  template <class MaxSATFormula>
  static void parseMaxSATFormula(gzFile input_stream,
                                 MaxSATFormula *maxsat_formula)
  {
    StreamBuffer in(input_stream);
    StreamBuffer read(input_stream);

    parseASPifFormula(in,maxsat_formula);
    //parseMaxSAT(in, maxsat_formula);
    if (maxsat_formula->getMaximumWeight() == 1)
      maxsat_formula->setProblemType(_UNWEIGHTED_);
    else
      maxsat_formula->setProblemType(_WEIGHTED_);

    // maxsat_formula->setInitialVars(maxsat_formula->nVars());
  }

  // New function to parse ASPif files
 template <class B, class MaxSATFormula>
static void parseASPifFormula(B &in, MaxSATFormula *maxsat_formula) {


    char *rule[100];
    int rule_size = 0;
    for(;;) {
        skipWhitespace(in);
        
        if(*in == EOF) {
            printf("Reached EOF\n");
            break;
        }
        else{
          char token[256];
          int token_index = 0;

          rule_size = 0;
          // Tokenize the line
          while (*in != '\n' && *in != EOF) {
              if (*in == ' ' || *in == '\t') {
                  if (token_index > 0) {
                      token[token_index] = '\0';
                      rule[rule_size] = strdup(token);  // Store token
                      rule_size++;
                      token_index = 0;
                  }
              } else {
                  token[token_index++] = *in;
              }
              ++in;
          }
          
          // Handle the last token
          if (token_index > 0) {
              token[token_index] = '\0';
              rule[rule_size] = strdup(token);
              rule_size++;
          }

          

          // Process the rule if it starts with "1"
          if (rule_size > 0 && strcmp(rule[0], "1") == 0) {
              // Determine if the rule is disjunctive
              int isDisjunctive = strcmp(rule[1], "0") == 0;
              if (!isDisjunctive) {
               // Extract the single head element
        int headElement = atoi(rule[3]);
        int var = abs(headElement) - 1;
        while (var >= maxsat_formula->nVars()) maxsat_formula->newVar();

        // Create the first rule: a :- not na.
        vec<Lit> lits1;
        lits1.push((headElement > 0) ? mkLit(var) : ~mkLit(var));
        int na_var = maxsat_formula->nVars();
        maxsat_formula->newVar();
        lits1.push(~mkLit(na_var));

        // Create the second rule: na :- not a.
        vec<Lit> lits2;
        lits2.push(mkLit(na_var));
        lits2.push(~((headElement > 0) ? mkLit(var) : ~mkLit(var)));

        // Check if there is a body
        if (rule_size > 5) {
            // Process and add the body elements to both rules
            for (int i = 6; i < rule_size; i++) {
                int bodyElement = atoi(rule[i]);
                int bodyVar = abs(bodyElement) - 1;
                while (bodyVar >= maxsat_formula->nVars()) maxsat_formula->newVar();
                lits1.push((bodyElement > 0) ? mkLit(bodyVar) : ~mkLit(bodyVar));
                lits2.push((bodyElement > 0) ? mkLit(bodyVar) : ~mkLit(bodyVar));
            }
        }
              maxsat_formula->addHardClause(lits1);
              maxsat_formula->addHardClause(lits2);

               printf("Converted choice rule {%d} into two rules using Clark's completion with body\n", headElement);
              }else{ int numHeadElements = atoi(rule[2]);
              char **headElements = &rule[3];
              printf("Extracted head elements\n");
              

              // Extract body elements
              int index = 3 + numHeadElements + 1;
              int numBodyElements = atoi(rule[index]);
              char **bodyElements = &rule[index + 1];
              printf("Extracted body elements\n");

              // Convert head and body elements to literals
                vec<Lit> lits;
                for (int i = 0; i < numHeadElements; i++) {
                    int var = abs(atoi(headElements[i])) - 1;
                    while (var >= maxsat_formula->nVars()) maxsat_formula->newVar();
                    lits.push((atoi(headElements[i]) > 0) ? mkLit(var) : ~mkLit(var));
                }
                for (int i = 0; i < numBodyElements; i++) {
                    int var = abs(atoi(bodyElements[i])) - 1;
                    while (var >= maxsat_formula->nVars()) maxsat_formula->newVar();
                    lits.push(~((atoi(bodyElements[i]) > 0) ? mkLit(var) : ~mkLit(var))); // Negate body elements
                }

                // Add the disjunction rule as a hard clause
                maxsat_formula->addHardClause(lits);
                printf("Added disjunction rule as a hard clause\n");
                
              

              // Debugging output (optional)
              printf("Rule type: disjunction\n");
              printf("Head: ");
              for (int i = 0; i < numHeadElements; i++) {
                  printf("%s ", headElements[i]);
              }
              printf("\n");
              

              if (numBodyElements > 0) {
                  printf("Body: ");
                  for (int i = 0; i < numBodyElements; i++) {
                      printf("%s ", bodyElements[i]);
                  }
                  printf("\n");
              }
              printf("\n");}    
          }
            else if (rule_size > 0 && strcmp(rule[0], "2") == 0) {
            // Parse the priority (p) - can be ignored
            int priority = atoi(rule[1]);

            // Parse the number of literals (n) in the rule
            int numLiterals = atoi(rule[2]);

            vec<Lit> minimizeLits;
            int index = 3;
            for (int i = 0; i < numLiterals; i++) {
                int var = abs(atoi(rule[index])) - 1;
                while (var >= maxsat_formula->nVars()) maxsat_formula->newVar();
                minimizeLits.push((atoi(rule[index]) > 0) ? mkLit(var) : ~mkLit(var));
                uint64_t weight = strtoull(rule[index + 1], NULL, 10); // Extract weight
                maxsat_formula->addSoftClause(weight, minimizeLits); // Add as soft clause with weight
                index += 2; // Move to the next literal
                printf("Added minimize statement as soft clauses with specific weights: %lu\n",weight);
                }            
        }
         else if (rule_size > 0 && strcmp(rule[0], "4") == 0) {
                int numChars = atoi(rule[1]);
                std::string atomName = rule[2];
                int numLiterals = atoi(rule[3]);
                int literalValue = atoi(rule[4]);
                
                // Store the atom name and its corresponding value
                aspifShow[literalValue]=atomName;

                printf("Stored output clause: %s with value %d\n", atomName.c_str(), literalValue);
            }
          // Free memory
          for (int i = 0; i < rule_size; i++) {
              free(rule[i]);
          }
          }
    }
    


    // Set problem type based on parsed data
    if (maxsat_formula->getMaximumWeight() == 1)
        maxsat_formula->setProblemType(_UNWEIGHTED_);
    else
        maxsat_formula->setProblemType(_WEIGHTED_);
    printf("Parsing complete.\n");
    
}
  
  //=================================================================================================
}

#endif // ParserMaxSAT_h