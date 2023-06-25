/*MIT License

Copyright (c) 2022 Shahryar Ahmad 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
#ifndef COMPILER_H_
#define COMPILER_H_
#include "plutonium.h"
#include "vm.h"
#include "lexer.h"
using namespace std;
#define JUMPOFFSet_Size 4
extern unordered_map<string,BuiltinFunc> funcs;
extern bool REPL_MODE;
extern Klass* TypeError;
extern Klass* ValueError;
extern Klass* MathError; 
extern Klass* NameError;
extern Klass* IndexError;
extern Klass* ArgumentError;
extern Klass* FileIOError;
extern Klass* KeyError;
extern Klass* OverflowError;
extern Klass* FileOpenError;
extern Klass* FileSeekError; 
extern Klass* ImportError;
extern Klass* ThrowError;
extern Klass* MaxRecursionError;
extern Klass* AccessError;
void REPL();


inline void addBytes(vector<uint8_t>& vec,int32_t x)
{
  size_t sz = vec.size();
  vec.resize(vec.size()+sizeof(int32_t));
  memcpy(&vec[sz],&x,sizeof(int32_t));
}
class Compiler
{
private:
  int32_t STACK_SIZE = 0;//simulating STACK's size
  int32_t scope = 0;
  size_t line_num = 1;
  vector<size_t> contTargets;
  vector<size_t> breakTargets;
  vector<int32_t> indexOfLastWhileLocals;
  vector<string>* fnReferenced;
  vector<std::unordered_map<string,int32_t>> locals;
  vector<string> prefixes;
  int32_t* num_of_constants;
  string filename;
  vector<string>* files;
  vector<string>* sources;
  short fileTOP;
  unordered_map<size_t,ByteSrc>* LineNumberTable;
  bool compileAllFuncs;
  vector<size_t> andJMPS;
  vector<size_t> orJMPS;
  //Context
  bool inConstructor = false;
  bool inGen = false;
  bool inclass = false;
  bool infunc = false;
  string className;
  vector<string> classMemb;
  int32_t foo = 0;
public:
  std::unordered_map<string,int32_t> globals;
  size_t bytes_done = 0;
  void init(vector<string>* fnR,int32_t* e,vector<string>* fnames,vector<string>* fsc,unordered_map<size_t,ByteSrc>* ltable,string filename)
  {
    fnReferenced = fnR;
    num_of_constants = e;
    files = fnames;
    sources = fsc;
    fileTOP = (short)(std::find(files->begin(),files->end(),filename) - files->begin());
    LineNumberTable = ltable;
    this->filename = filename;
  }
  void compileError(string type,string msg)
  {
    fprintf(stderr,"\nFile %s\n",filename.c_str());
    fprintf(stderr,"%s at line %ld\n",type.c_str(),line_num);
    auto it = std::find(files->begin(),files->end(),filename);
    size_t i = it-files->begin();
    string& source_code = (*sources)[i];
    size_t l = 1;
    string line = "";
    size_t k = 0;
    while(l<=line_num)
    {
        if(source_code[k]=='\n')
            l+=1;
        else if(l==line_num)
            line+=source_code[k];
        k+=1;
        if(k>=source_code.length())
        {
          break;
        }
    }
    fprintf(stderr,"%s\n",lstrip(line).c_str());
    fprintf(stderr,"%s\n",msg.c_str());
    if(REPL_MODE)
      REPL();
    exit(0);
  }
  int32_t isDuplicateConstant(PltObject x)
  {
      for (int32_t k = 0; k < vm.total_constants; k += 1)
      {
          if (vm.constants[k] == x)
              return k;
      }
      return -1;
  }
  int32_t addToVMStringTable(const string& n)
  {
    auto it = std::find(vm.strings.begin(),vm.strings.end(),n);
    if(it==vm.strings.end())
    {
        vm.strings.push_back(n);
        return (int32_t)vm.strings.size()-1;
    }
    else
      return (int32_t)(it - vm.strings.begin());
  }
  inline void addLnTableEntry(size_t opcodeIdx)
  {
    ByteSrc tmp = {fileTOP,line_num};
    LineNumberTable->emplace(opcodeIdx,tmp);
  }
  vector<uint8_t> exprByteCode(Node* ast)
  {
      PltObject reg;
      vector<uint8_t> bytes;
      if (ast->childs.size() == 0)
      {
          if (ast->type == NodeType::NUM)
          {
              const string& n = ast->val;
              if (isnum(n))
              {
                  reg.type = 'i';
                  reg.i = Int(n);
                  int32_t e = isDuplicateConstant(reg);
                  if (e != -1)
                      foo = e;
                  else
                  {
                      foo = vm.total_constants;
                      reg.type = 'i';
                      reg.i = Int(n);
                      vm.constants[vm.total_constants++] = reg;
                  }
                  bytes.push_back(LOAD_CONST);
                  addBytes(bytes,foo);
                  bytes_done += 1 + sizeof(int32_t);
              }
              else if (isInt64(n))
              {
                  reg.type = 'l';
                  reg.l = toInt64(n);
                  int32_t e = isDuplicateConstant(reg);
                  if (e != -1)
                      foo = e;
                  else
                  {
                      foo = vm.total_constants;
                      reg.type = 'l';
                      reg.l = toInt64(n);
                      vm.constants[vm.total_constants++] = reg;
                  }
                  bytes.push_back(LOAD_CONST);
                  addBytes(bytes,foo);
                  bytes_done += 1 + sizeof(int32_t);
              }
              else
              {
                  compileError("OverflowError", "Error integer "+n+" causes overflow!");
              }
              return bytes;
          }
          else if (ast->type == NodeType::FLOAT)
          {
              string n = ast->val;
              bool neg = false;
              if (n[0] == '-')
              {
                  neg = true;
                  n = n.substr(1);
              }
             /* while (n.length() > 0 && n[n.length() - 1] == '0')
              {
                  n = n.substr(0, n.length() - 1);
              }*/
              if (neg)
                  n = "-" + n;
              reg.type = 'f';
              reg.f = Float(n);
              int32_t e = isDuplicateConstant(reg);
              if (e != -1)
                  foo = e;
              else
              {
                  foo = vm.total_constants;

                  vm.constants[vm.total_constants++] = reg;
              }
              bytes.push_back(LOAD_CONST);
              addBytes(bytes,foo);
              bytes_done += 1 + sizeof(int32_t);
              return bytes;
          }
          else if (ast->type ==  NodeType::BOOL)
          {
              bytes.push_back(LOAD_CONST);
              string n = ast->val;
              bool a = (n == "true") ? true : false;
              reg.type = 'b';
              reg.i = a;
              int32_t e = isDuplicateConstant(reg);
              if (e != -1)
                  foo = e;
              else
              {
                  foo = vm.total_constants;
                  vm.constants[vm.total_constants++] = reg;
              }
              addBytes(bytes,foo);
              bytes_done += 1 + sizeof(int32_t);
              return bytes;
          }
          else if (ast->type == NodeType::STR)
          {
              bytes.push_back(LOAD_STR);
              const string& n = ast->val;
              foo = addToVMStringTable(n);
              addBytes(bytes,foo);
              bytes_done += 5;
              return bytes;
          }
          else if (ast->type == NodeType::NIL)
          {
              bytes.push_back(LOAD_CONST);
              PltObject ret;
              foo = 0;
              addBytes(bytes,foo);
              bytes_done += 5;
              return bytes;
          }
          else if (ast->type == NodeType::ID)
          {
              const string& name = ast->val;
              bool isGlobal = false;
              bool isSelf = false;
              foo = resolveName(name,isGlobal,true,&isSelf);
              if(isSelf)
              {
                addLnTableEntry(bytes_done);
                bytes.push_back(SELFMEMB);
                foo = addToVMStringTable(name);
                addBytes(bytes,foo);
                bytes_done+=5;
                return bytes;
              }
              else if(!isGlobal)
              {
                bytes.push_back(LOAD_LOCAL);
                addBytes(bytes,foo);
                bytes_done+=5;
                return bytes;
              }
              else
              {
                bytes.push_back(LOAD_GLOBAL);
                addBytes(bytes,foo);
                bytes_done += 5;
                return bytes;
              }
          }
          else if (ast->type == NodeType::BYTE)
          {
              bytes.push_back(LOAD_CONST);
              uint8_t b = tobyte(ast->val);
              reg.type = 'm';
              reg.i = b;
              int32_t e = isDuplicateConstant(reg);
              if (e != -1)
                  foo = e;
              else
              {
                  foo = vm.total_constants;
                  vm.constants[vm.total_constants++] = reg;
              }
              addBytes(bytes,foo);
              bytes_done += 1 + sizeof(int32_t);
              return bytes;
          }

      }
      if (ast->type == NodeType::list)
      {
          
          for (size_t k = 0; k < ast->childs.size(); k += 1)
          {
              vector<uint8_t> elem = exprByteCode(ast->childs[k]);
              bytes.insert(bytes.end(), elem.begin(), elem.end());
          }
          bytes.push_back(LOAD);
          bytes.push_back('j');
          foo = ast->childs.size();
          addBytes(bytes,foo);
          bytes_done += 2 + 4;
          return bytes;
      }
      if (ast->type == NodeType::dict)
      {
          
          for (size_t k = 0; k < ast->childs.size(); k += 1)
          {
              vector<uint8_t> elem = exprByteCode(ast->childs[k]);
              bytes.insert(bytes.end(), elem.begin(), elem.end());
          }
          addLnTableEntry(bytes_done);//runtime errors can occur
          bytes.push_back(LOAD);
          bytes.push_back('a');
          foo = ast->childs.size() / 2;//number of key value pairs in dictionary
          addBytes(bytes,foo);
          bytes_done += 2 + 4;
          return bytes;
      }

      if (ast->type == NodeType::add)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(ADD);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::sub)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(SUB);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::div)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(DIV);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::mul)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(MUL);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::XOR)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(XOR);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::mod)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(MOD);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::lshift)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(LSHIFT);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::rshift)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(RSHIFT);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::bitwiseand)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(BITWISE_AND);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }

      if (ast->type == NodeType::bitwiseor)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(BITWISE_OR);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::memb)
      {
          
          if (ast->childs[1]->type == NodeType::call)
          {
              Node* callnode = ast->childs[1];

              vector<uint8_t> a = exprByteCode(ast->childs[0]);
              bytes.insert(bytes.end(), a.begin(), a.end());
              Node* args = callnode->childs[2];
              for (size_t f = 0; f < args->childs.size(); f += 1)
              {
                  vector<uint8_t> arg = exprByteCode(args->childs[f]);
                  bytes.insert(bytes.end(), arg.begin(), arg.end());
              }
                bytes.push_back(CALLMETHOD);
              addLnTableEntry(bytes_done);
              const string& memberName = callnode->childs[1]->val;
              foo = addToVMStringTable(memberName);
              addBytes(bytes,foo);
              bytes.push_back(args->childs.size());
              
              bytes_done +=6;

              return bytes;
          }
          else
          {
              vector<uint8_t> a = exprByteCode(ast->childs[0]);
              bytes.insert(bytes.end(), a.begin(), a.end());
              addLnTableEntry(bytes_done);
              bytes.push_back(MEMB);
              if (ast->childs[1]->type != NodeType::ID)
                  compileError("SyntaxError", "Invalid Syntax");
              const string& name = ast->childs[1]->val;
              foo = addToVMStringTable(name);
              addBytes(bytes,foo);
              bytes_done +=5;
              return bytes;
          }
      }
      if (ast->type == NodeType::AND)
      {
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());

          bytes.push_back(JMPIFFALSENOPOP);
          andJMPS.push_back(bytes_done);
       //   printf("added andJMP at %ld\n",bytes_done);
          int32_t I = bytes.size();
          bytes.push_back(0);
          bytes.push_back(0);
          bytes.push_back(0);
          bytes.push_back(0);
          bytes_done+=5;
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());

          foo = b.size();
          memcpy(&bytes[I],&foo,sizeof(int32_t));

          return bytes;
      }
      if (ast->type == NodeType::IS)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(IS);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::OR)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          bytes.push_back(NOPOPJMPIF);
          orJMPS.push_back(bytes_done);
          int32_t I = bytes.size();
          bytes.push_back(0);
          bytes.push_back(0);
          bytes.push_back(0);
          bytes.push_back(0);
          bytes_done+=5;
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());

          foo = b.size();
          memcpy(&bytes[I],&foo,sizeof(int32_t));
          return bytes;
      }
      if (ast->type == NodeType::lt)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(SMALLERTHAN);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::gt)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(GREATERTHAN);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::equal)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(EQ);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::noteq)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(NOTEQ);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::gte)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(GROREQ);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::lte)
      {
          
          vector<uint8_t> a = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), a.begin(), a.end());
          vector<uint8_t> b = exprByteCode(ast->childs[1]);
          bytes.insert(bytes.end(), b.begin(), b.end());
          bytes.push_back(SMOREQ);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::neg)
      {
          
          vector<uint8_t> val = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), val.begin(), val.end());
          bytes.push_back(NEG);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::complement)
      {
          
          vector<uint8_t> val = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), val.begin(), val.end());
          bytes.push_back(COMPLEMENT);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::NOT)
      {
          
          vector<uint8_t> val = exprByteCode(ast->childs[0]);
          bytes.insert(bytes.end(), val.begin(), val.end());
          bytes.push_back(NOT);
          addLnTableEntry(bytes_done);
          bytes_done += 1;
          return bytes;
      }
      if (ast->type == NodeType::index)
      {
        
        vector<uint8_t> a = exprByteCode(ast->childs[0]);
        bytes.insert(bytes.end(), a.begin(), a.end());
        vector<uint8_t> b = exprByteCode(ast->childs[1]);
        bytes.insert(bytes.end(), b.begin(), b.end());
        bytes.push_back(INDEX);
        addLnTableEntry(bytes_done);
        bytes_done += 1;
        return bytes;
      }
      if (ast->type == NodeType::call)
      {
          const string& name = ast->childs[1]->val;
          bool udf = false;
          if (funcs.find(name) == funcs.end())//check if it is builtin function
            udf = true;
          if (udf)
          {
              bool isGlobal = false;
              bool isSelf = false;
              int foo = resolveName(name,isGlobal,true,&isSelf);
              if(isSelf)
              {
                bytes.push_back(LOAD_LOCAL);
                addBytes(bytes,0);//load self to pass as first argument
                bytes_done+=5;
              }
              for (size_t k = 0; k < ast->childs[2]->childs.size(); k += 1)
              {
                  vector<uint8_t> val = exprByteCode(ast->childs[2]->childs[k]);
                  bytes.insert(bytes.end(), val.begin(), val.end());
              }
              if(isSelf)
              {
                addLnTableEntry(bytes_done);
                bytes.push_back(SELFMEMB);
                foo = addToVMStringTable(name);
                addBytes(bytes,foo);
                bytes_done+=5;
                addLnTableEntry(bytes_done);
                bytes.push_back(CALLUDF);
                bytes.push_back(ast->childs[2]->childs.size()+1);
                bytes_done+=2;
                return bytes;
              }
              else if(isGlobal)
                bytes.push_back(LOAD_GLOBAL);
              else
                bytes.push_back(LOAD_LOCAL);
              addBytes(bytes,foo);
              bytes_done+=5;
              addLnTableEntry(bytes_done);
              bytes.push_back(CALLUDF);
              bytes.push_back(ast->childs[2]->childs.size());
              bytes_done+=2;
              return bytes;
          }
          else
          {
              for (size_t k = 0; k < ast->childs[2]->childs.size(); k += 1)
              {
                  vector<uint8_t> val = exprByteCode(ast->childs[2]->childs[k]);
                  bytes.insert(bytes.end(), val.begin(), val.end());
              }
              addLnTableEntry(bytes_done);
              bytes.push_back(CALLFORVAL);
              bool add = true;
              size_t index = 0;
              for(index = 0;index < vm.builtin.size();index+=1)
              {
                if(vm.builtin[index]==funcs[name])
                {
                  add = false;
                  break;
                }
              }
              if(add)
              {
                vm.builtin.push_back(funcs[name]);
                foo = vm.builtin.size()-1;
              }
              else
                foo = index;
              addBytes(bytes,foo);
              bytes.push_back((char)ast->childs[2]->childs.size());
              bytes_done += 6;
              return bytes;
          }
      }
      if (ast->type == NodeType::YIELD)
      {
        if(inConstructor)
          compileError("SyntaxError","Error class constructor can not be generators!");
        
        vector<uint8_t> val = exprByteCode(ast->childs[1]);
        bytes.insert(bytes.end(), val.begin(), val.end());              
        addLnTableEntry(bytes_done);
        bytes.push_back(YIELD_AND_EXPECTVAL);
        bytes_done += 1;
        return bytes;
      }  
      compileError("SyntaxError", "Invalid syntax in expression");
      exit(0);
      return bytes;//to prevent compiler warning
  }
  
  vector<string> scanClass(Node* ast)
  {
    vector<string> names;
    while(ast->val!="endclass")
    {
      if(ast->type==NodeType::declare)
      {
        string n = ast->val;
        if(n[0]=='@')
         n = n.substr(1);
        if(std::find(names.begin(),names.end(),n)!=names.end() || std::find(names.begin(),names.end(),"@"+n)!=names.end())
        {
          line_num = atoi(ast->childs[0]->val.c_str());
          compileError("NameError","Error redeclaration of "+n+".");
        }
        names.push_back(ast->val);
      }
      else if(ast->type == NodeType::FUNC)
      {

        string n = ast->childs[1]->val;
        if(n[0]=='@')
         n = n.substr(1);
        if(std::find(names.begin(),names.end(),n)!=names.end() || std::find(names.begin(),names.end(),"@"+n)!=names.end())
        {
          line_num = atoi(ast->childs[0]->val.c_str());
          compileError("NameError","Error redeclaration of "+n+".");
        }
        names.push_back(ast->childs[1]->val);
      }
      else if(ast->type == NodeType::CORO) //generator function or coroutine
      {
          line_num = atoi(ast->childs[0]->val.c_str());
          compileError("NameError","Error coroutine inside class not allowed.");
      }
      
      else if(ast->type==NodeType::CLASS)
      {
        line_num = atoi(ast->childs[0]->val.c_str());
        compileError("SyntaxError","Error nested classes not supported");     
      }
      ast = ast->childs.back();
    }
    return names;
  }
  int32_t resolveName(string name,bool& isGlobal,bool blowUp=true,bool* isFromSelf=NULL)
  {
    for(int32_t i=locals.size()-1;i>=0;i-=1)
    {
      if(locals[i].find(name)!=locals[i].end())
        return locals[i][name];
    }
    for(int32_t i=prefixes.size()-1;i>=0;i--)
      {
        string prefix = prefixes[i];
        if(globals.find(prefix+name) != globals.end())
        {
            isGlobal = true;
            return globals[prefix+name];
        }
      }
    if(isFromSelf)
    {
      if(inclass && infunc && (std::find(classMemb.begin(),classMemb.end(),name)!=classMemb.end() || std::find(classMemb.begin(),classMemb.end(),"@"+name)!=classMemb.end() ))
      {
        *isFromSelf = true;
        return -2;
      }
    }
    if(globals.find(name) != globals.end())
    {
        isGlobal = true;
        return globals[name];
    }
    if(blowUp)
      compileError("NameError","Error name "+name+" is not defined!");
    return -1;
  }
  
  vector<uint8_t> compile(Node* ast)
  {
      vector<uint8_t> program;
      bool isGen = false;
      while (ast->type != NodeType::EOP && ast->val!="endclass" && ast->val!="endfor" && ast->val!="endnm" && ast->val!="endtry" && ast->val!="endcatch" && ast->val != "endif" && ast->val != "endfunc" && ast->val != "endelif" && ast->val != "endwhile" && ast->val != "endelse")
      {
          if(ast->childs.size() >= 1)
          {
            if (ast->childs[0]->type == NodeType::line)
              line_num = Int(ast->childs[0]->val);
          }
          if (ast->type == NodeType::declare)
          {
              vector<uint8_t> val = exprByteCode(ast->childs[1]);
              program.insert(program.end(), val.begin(), val.end());
              const string& name = ast->val;
              
              if (scope == 0)
              {
                  if (globals.find(name) != globals.end())
                    compileError("NameError", "Error redeclaration of variable " + name);
                  foo = STACK_SIZE;
                  if(!inclass)
                    globals.emplace(name,foo);
                  STACK_SIZE+=1;
              }
              else
              {
                  if(locals.back().find(name)!=locals.back().end())
                      compileError("NameError", "Error redeclaration of variable " + name);
                  foo = STACK_SIZE;
                  if(inclass && !infunc);
                  else
                    locals.back().emplace(name,foo);
                  STACK_SIZE+=1;
              }

          }
          else if (ast->type == NodeType::import || ast->type==NodeType::importas)
          {
              string name = ast->childs[1]->val;
              string vname = (ast->type==NodeType::importas) ? ast->childs[2]->val : name;
              bool f;
              if(resolveName(vname,f,false)!=-1)
                compileError("NameError","Error redeclaration of name "+vname);
              addLnTableEntry(bytes_done);
              program.push_back(IMPORT);
              foo = addToVMStringTable(name);
              addBytes(program,foo);
              bytes_done += 5;
              if(scope==0)
                globals.emplace(vname,STACK_SIZE);
              else
                locals.back().emplace(vname,STACK_SIZE);
              STACK_SIZE+=1;
          }
          else if (ast->type == NodeType::assign)
          {
              
              string name = ast->childs[1]->val;
              bool doit = true;
              if (ast->childs[1]->type == NodeType::ID)
              {
                  if (ast->childs[2]->childs.size() == 2)
                  {
                      if (ast->childs[2]->type == NodeType::add && ast->childs[2]->childs[0]->val == ast->childs[1]->val && ast->childs[2]->childs[0]->type==NodeType::ID && ast->childs[2]->childs[1]->type==NodeType::NUM && ast->childs[2]->childs[1]->val == "1")
                      {
                        bool isGlobal = false;
                        bool isSelf = false;
                        int32_t idx = resolveName(name,isGlobal,false,&isSelf);
                        if(idx==-1)
                          compileError("NameError","Error name "+name+" is not defined!");
                        if(isSelf)
                        {
                          vector<uint8_t> val = exprByteCode(ast->childs[2]);
                          program.insert(program.end(), val.begin(), val.end());
                        }
                        addLnTableEntry(bytes_done);
                        foo = idx;
                        if(isSelf)
                        {
                          foo = addToVMStringTable(name);
                          program.push_back(ASSIGNSELFMEMB);
                        }
                        else if(!isGlobal)
                          program.push_back(INPLACE_INC);
                        else
                          program.push_back(INC_GLOBAL);
                        addBytes(program,foo);
                        bytes_done+=5;
                        doit = false;
                      }
                  }
                  if (doit)
                  {
                      bool isGlobal = false;
                      bool isSelf = false;
                      int32_t idx = resolveName(name,isGlobal,false,&isSelf);
                      if(idx==-1)
                         compileError("NameError","Error name "+name+" is not defined!");
                      vector<uint8_t> val = exprByteCode(ast->childs[2]);
                      program.insert(program.end(), val.begin(), val.end());
                      addLnTableEntry(bytes_done);
                      if(isSelf)
                      {
                        idx = addToVMStringTable(name);
                        program.push_back(ASSIGNSELFMEMB);
                      }
                      else if(!isGlobal)
                        program.push_back(ASSIGN);
                      else
                        program.push_back(ASSIGN_GLOBAL);
                      addBytes(program,idx);
                      bytes_done += 5;
                  }
              }
              else if (ast->childs[1]->type == NodeType::index)
              {
                  //reassign index
                  vector<uint8_t> M = exprByteCode(ast->childs[1]->childs[0]);
                  program.insert(program.end(),M.begin(),M.end());
                  M = exprByteCode(ast->childs[2]);
                  program.insert(program.end(),M.begin(),M.end());
                  M = exprByteCode(ast->childs[1]->childs[1]);
                  program.insert(program.end(),M.begin(),M.end());
                  program.push_back(ASSIGNINDEX);
                  addLnTableEntry(bytes_done);
                  bytes_done+=1;
              }
              else if (ast->childs[1]->type == NodeType::memb)
              {
                  //reassign object member
                  vector<uint8_t> lhs = exprByteCode(ast->childs[1]->childs[0]);
                  program.insert(program.end(),lhs.begin(),lhs.end());
                  if(ast->childs[1]->childs[1]->type!=NodeType::ID)
                    compileError("SyntaxError","Invalid Syntax");
                  string mname = ast->childs[1]->childs[1]->val;
                  vector<uint8_t> val = exprByteCode(ast->childs[2]);
                  program.insert(program.end(), val.begin(), val.end());
                  program.push_back(ASSIGNMEMB);
                  foo = addToVMStringTable(mname);
                  addBytes(program,foo);
                  addLnTableEntry(bytes_done);
                  bytes_done+=5;

              }
          }
          else if (ast->type == NodeType::memb)
          {
              Node* bitch = NewNode(NodeType::memb,".");
              bitch->childs.push_back(ast->childs[1]);
              bitch->childs.push_back(ast->childs[2]);
              vector<uint8_t> stmt = exprByteCode(bitch);
              delete bitch;
              program.insert(program.end(),stmt.begin(),stmt.end());
              program.push_back(POP_STACK);
              bytes_done +=1;
          }
          else if (ast->type == NodeType::WHILE || ast->type == NodeType::DOWHILE)
          {
              
              if(ast->type==NodeType::DOWHILE)
                   bytes_done+=5;//do while uses one extra jump before the condition
              contTargets.push_back(bytes_done);
              size_t L = bytes_done;
              vector<uint8_t> cond = exprByteCode(ast->childs[1]);
              breakTargets.push_back(bytes_done);

              bytes_done += 1 + JUMPOFFSet_Size;//JMPFALSE <4 byte where>

              indexOfLastWhileLocals.push_back(locals.size());
              scope += 1;
              int32_t before = STACK_SIZE;
              std::unordered_map<string,int32_t> m;
              locals.push_back(m);

              vector<uint8_t> block = compile(ast->childs[2]);

              STACK_SIZE = before;
              breakTargets.pop_back();
              contTargets.pop_back();
              scope -= 1;
              indexOfLastWhileLocals.pop_back();
              int32_t whileLocals = locals.back().size();
              locals.pop_back();
              int32_t where = block.size() + 1 + JUMPOFFSet_Size;
              if(whileLocals!=0)
                where+=4;//4 for NPOP_STACK
             if(ast->type==NodeType::DOWHILE)
             {
                program.push_back(JMP);
                foo = cond.size()+ 5;
                addBytes(program,foo);
              }
              program.insert(program.end(), cond.begin(), cond.end());
              //Check while condition
              program.push_back(JMPIFFALSE);
              foo = where;
              addBytes(program,foo);

              //
              program.insert(program.end(), block.begin(), block.end());
              if(whileLocals!=0)
              {
                program.push_back(GOTONPSTACK);
                foo = whileLocals;
                addBytes(program,foo);
                bytes_done += 4;
              }
              else
                program.push_back(GOTO);
              foo = L;
              addBytes(program,foo);
              bytes_done += 1 + JUMPOFFSet_Size;

          }
          else if (ast->type == NodeType::FOR )
          {
              
            size_t lnCopy = line_num;
            bool decl = ast->childs[1]->type == NodeType::decl;
            size_t L = bytes_done;
            const string& loop_var_name = ast->childs[2]->val;
            string I = ast->childs[5]->val;
            vector<uint8_t> initValue = exprByteCode(ast->childs[3]);

            program.insert(program.end(),initValue.begin(),initValue.end());
            bool isGlobal = false;
            int32_t lcvIdx ;
            if(!decl)
            {
              bool isSelf = false;
              foo = resolveName(loop_var_name,isGlobal,true,&isSelf);
              lcvIdx = foo;
              if(isSelf)
              {
                addLnTableEntry(bytes_done);
                program.push_back(ASSIGNSELFMEMB);
                foo = addToVMStringTable(loop_var_name);
                L+=5;
              }
              else if(!isGlobal)
              {
                program.push_back(ASSIGN);
                L+=5;
              }
              else
                program.push_back(ASSIGN_GLOBAL);
              addBytes(program,foo);
              bytes_done+=5;

            }
            contTargets.push_back(bytes_done);
            bytes_done+=5;

            vector<uint8_t> finalValue = exprByteCode(ast->childs[4]);
            //
            breakTargets.push_back(bytes_done+1);
            int32_t before = STACK_SIZE;
            if(decl)
            {
              scope += 1;
              before = STACK_SIZE+1;
              std::unordered_map<string,int32_t> m;
              scope+=1;
              locals.push_back(m);
              lcvIdx = STACK_SIZE;
              locals.back().emplace(loop_var_name,STACK_SIZE);
              STACK_SIZE+=1;
              indexOfLastWhileLocals.push_back(locals.size());
              locals.push_back(m);
            }
            else
            {
              scope += 1;
              std::unordered_map<string,int32_t> m;
              indexOfLastWhileLocals.push_back(locals.size());
              locals.push_back(m);
            }

            bytes_done+=2+JUMPOFFSet_Size;//for jump before block
            vector<uint8_t> block = compile(ast->childs[6]);

            STACK_SIZE = before;
            breakTargets.pop_back();
            contTargets.pop_back();
            if(decl)
              scope -= 2;
            else
              scope-=1;
            indexOfLastWhileLocals.pop_back();
            int32_t whileLocals = locals.back().size();
            locals.pop_back();
            if(decl)
              locals.pop_back();
            int32_t where;
            if(!isGlobal)
            {
              program.push_back(LOAD_LOCAL);
            }
            else
              program.push_back(LOAD_GLOBAL);
            foo = lcvIdx;
            addBytes(program,foo);//load loop control variable
            program.insert(program.end(),finalValue.begin(),finalValue.end());
            program.push_back(SMOREQ);

            ////
            vector<uint8_t> inc;
            line_num = lnCopy;


            if(I!="1")
            {
              inc = exprByteCode(ast->childs[5]);
              where = block.size() + 1 + JUMPOFFSet_Size+inc.size()+12;
              if(isGlobal)
                where-=1;
            }
            else
            {
              where = block.size() + 1 + JUMPOFFSet_Size+5;

            }
            if(whileLocals!=0)
              where+=4;//4 for NPOP_STACK
            ////
            //Check condition
            program.push_back(JMPIFFALSE);
            foo = where;
            addBytes(program,foo);
            //
            block.insert(block.end(),inc.begin(),inc.end());
            if(I=="1" && ast->childs[5]->type == NodeType::NUM)
            {
              if(!isGlobal)
                block.push_back(INPLACE_INC);
              else
                block.push_back(INC_GLOBAL);
              foo = lcvIdx;
              addBytes(block,foo);
              bytes_done+=5;
            }
            else
            {
              if(!isGlobal)
              {
                block.push_back(LOAD_LOCAL);

              }
              else
                block.push_back(LOAD_GLOBAL);
              foo = lcvIdx;
              addBytes(block,foo);
              block.push_back(ADD);
              if(!isGlobal )
                block.push_back(ASSIGN);
              else
                block.push_back(ASSIGN_GLOBAL);
              addBytes(block,foo);


                bytes_done+=11;

            }
            program.insert(program.end(), block.begin(), block.end());
            if(whileLocals!=0)
            {
              program.push_back(GOTONPSTACK);
              foo = whileLocals;
              addBytes(program,foo);
              bytes_done += 4;
            }
            else
              program.push_back(GOTO);
            foo = L+initValue.size();
            if(isGlobal)
              foo+=5;
            addBytes(program,foo);
            if(decl)
            {
              program.push_back(POP_STACK);
              bytes_done+=1;
              STACK_SIZE-=1;
            }
            bytes_done += 1 + JUMPOFFSet_Size;
          }
          else if (ast->type == NodeType::FOREACH)
          {
              
              string loop_var_name = ast->childs[1]->val;

              vector<uint8_t> startIdx = exprByteCode(ast->childs[3]);
              program.insert(program.end(),startIdx.begin(),startIdx.end());
              int32_t E = STACK_SIZE;
              STACK_SIZE+=1;

              std::unordered_map<string,int32_t> m;
              bool add = true;
              size_t index = 0;
              int32_t fnIdx = 0;
              for(index = 0;index < vm.builtin.size();index+=1)
              {
                if(vm.builtin[index]==funcs["len"])
                {
                  add = false;
                  break;
                }
              }
              if(add)
              {
                vm.builtin.push_back(funcs["len"]);
                fnIdx = vm.builtin.size()-1;
              }
              else
                fnIdx = index;
              //Bytecode to calculate length of list we are looping
              vector<uint8_t> LIST = exprByteCode(ast->childs[2]);
              program.insert(program.end(),LIST.begin(),LIST.end());
              int32_t ListStackIdx = STACK_SIZE;
                STACK_SIZE+=1;
              foo = E;
              program.push_back(INPLACE_INC);
              addBytes(program,foo);
              contTargets.push_back(bytes_done);
              bytes_done+=5;
  //////////
              int32_t before = STACK_SIZE;
              scope+=1;
              indexOfLastWhileLocals.push_back(locals.size());
              locals.push_back(m);
              locals.back().emplace(loop_var_name,STACK_SIZE);
              STACK_SIZE+=1;
              size_t L = bytes_done;
              addLnTableEntry(L+12);
              bytes_done+=20+6+1+JUMPOFFSet_Size+6;
              breakTargets.push_back(bytes_done-1-JUMPOFFSet_Size-7-6);
              vector<uint8_t> block = compile(ast->childs[4]);
              program.push_back(LOAD);
              program.push_back('v');
              foo = E;
              addBytes(program,foo);

              //
              program.push_back(LOAD);
              program.push_back('v');
              foo = ListStackIdx;
              addBytes(program,foo);
              //

              program.push_back(CALLFORVAL);
              foo = fnIdx;
              addBytes(program,foo);
              program.push_back(1);
              program.push_back(SMALLERTHAN);
              program.push_back(JMPIFFALSE);
              int32_t where = 7+block.size()+1+JUMPOFFSet_Size+5+6;
              int32_t whileLocals = locals.back().size();
              if(whileLocals!=0)
                where+=4;
              foo = where;
              addBytes(program,foo);

            //  program.insert(program.end(),LIST.begin(),LIST.end());
              foo = ListStackIdx;
              program.push_back(LOAD);
              program.push_back('v');
              addBytes(program,foo);
              program.push_back(LOAD);
              program.push_back('v');
              foo = E;
              addBytes(program,foo);
              program.push_back(INDEX);
              program.insert(program.end(),block.begin(),block.end());
              program.push_back(INPLACE_INC);
              foo = E;
              addBytes(program,foo);
              bytes_done+=5;
              STACK_SIZE = before-2;
              breakTargets.pop_back();
              contTargets.pop_back();
              scope -= 1;
              indexOfLastWhileLocals.pop_back();

              locals.pop_back();
              if(whileLocals!=0)
              {
                program.push_back(GOTONPSTACK);
                foo = whileLocals;
                addBytes(program,foo);
                bytes_done += 4;
              }
              else
                program.push_back(GOTO);
              foo = L;
              addBytes(program,foo);
              program.push_back(NPOP_STACK);
              foo = 2;
              addBytes(program,foo);
              bytes_done += 1 + JUMPOFFSet_Size+JUMPOFFSet_Size+1;
          }
          else if(ast->type == NodeType::NAMESPACE)
          {
            if(infunc)
            {
              compileError("SyntaxError","Namespace declartion inside function!");
            }
            string name = ast->childs[1]->val;
            string prefix;
            for(auto e: prefixes)
              prefix+=e;
            prefixes.push_back(prefix+name+"::");
            vector<uint8_t> block = compile(ast->childs[2]);
            prefixes.pop_back();
            program.insert(program.end(),block.begin(),block.end());
            //Hasta La Vista Baby
          }
          else if (ast->type == NodeType::IF)
          {
              
              vector<uint8_t> cond = exprByteCode(ast->childs[1]->childs[0]);
              if(ast->childs[2]->val=="endif")
              {
                program.insert(program.end(),cond.begin(),cond.end());
                program.push_back(POP_STACK);
                bytes_done+=1;
                ast = ast->childs.back();
                continue;
              }
              bytes_done += 1 + JUMPOFFSet_Size;
              scope += 1;
              int32_t before = STACK_SIZE;
              std::unordered_map<string,int32_t> m;
              locals.push_back(m);
              vector<uint8_t> block = compile(ast->childs[2]);
              scope -= 1;
              STACK_SIZE = before;
              bool hasLocals = (locals.back().size()!=0);

              program.insert(program.end(), cond.begin(), cond.end());
              program.push_back(JMPIFFALSE);
              foo = block.size();//
              if(hasLocals)
                foo+=5;
              addBytes(program,foo);
              program.insert(program.end(), block.begin(), block.end());
              if(hasLocals)
              {
                program.push_back(NPOP_STACK);
                foo = locals.back().size();
                addBytes(program,foo);
                bytes_done += 5;
              }
              locals.pop_back();
          }
          else if (ast->type == NodeType::IFELSE)
          {
              
              vector<uint8_t> ifcond = exprByteCode(ast->childs[1]->childs[0]);
              bytes_done += 1 + JUMPOFFSet_Size;
              scope += 1;
              int32_t before = STACK_SIZE;
              std::unordered_map<string,int32_t> m;
              locals.push_back(m);
              vector<uint8_t> ifblock = compile(ast->childs[2]);
              scope -= 1;
              STACK_SIZE = before;
              //
              int32_t iflocals = locals.back().size();
              locals.pop_back();
              program.insert(program.end(), ifcond.begin(), ifcond.end());
              program.push_back(JMPIFFALSE);
              foo = ifblock.size() + 1 + JUMPOFFSet_Size;

              if(iflocals!=0)
              {
                foo+=4;
                bytes_done+=4;
              }
              addBytes(program,foo);
              program.insert(program.end(), ifblock.begin(), ifblock.end());


              scope += 1;
              bytes_done += 1 + JUMPOFFSet_Size;
              before = STACK_SIZE;
              locals.push_back(m);
              vector<uint8_t> elseblock = compile(ast->childs[3]);
              scope -= 1;
              STACK_SIZE = before;
              int32_t elseLocals = locals.back().size();
              locals.pop_back();
              if(iflocals!=0)
              {
                program.push_back(JMPNPOPSTACK);
                foo = iflocals;
                addBytes(program,foo);
              }
              else
                program.push_back(JMP);
              foo = elseblock.size();
              if(elseLocals!=0)
                foo+=5;
              addBytes(program,foo);
              program.insert(program.end(), elseblock.begin(), elseblock.end());
              if(elseLocals!=0)
              {
                program.push_back(NPOP_STACK);
                foo = elseLocals;
                addBytes(program,foo);
                bytes_done += 5;
              }
          }
          else if (ast->type == NodeType::IFELIFELSE)
          {
              
              vector<uint8_t> ifcond = exprByteCode(ast->childs[1]->childs[0]);
              bytes_done += 1 + JUMPOFFSet_Size;//JumpIfFalse after if condition
              scope += 1;
              std::unordered_map<string,int32_t> m;
              locals.push_back(m);
              int32_t before = STACK_SIZE;
              vector<uint8_t> ifblock = compile(ast->childs[2]);
              scope -= 1;
              STACK_SIZE = before;
              int32_t iflocals = locals.back().size();
              locals.pop_back();
              bytes_done += 1 + JUMPOFFSet_Size;//JMP of if block
              if(iflocals!=0)
                bytes_done+=4;//for NPOP_STACK+ 4 byte operand
              vector<vector<uint8_t>> elifConditions;
              int32_t elifBlocksSize = 0;//total size in bytes elif blocks take including their conditions
              int32_t elifBlockcounter = 3;//third node of ast
              vector<int32_t> elifLocalSizes ;
              vector<vector<uint8_t>> elifBlocks;
              for (size_t k = 1; k < ast->childs[1]->childs.size(); k += 1)
              {
                  vector<uint8_t> elifCond = exprByteCode(ast->childs[1]->childs[k]);
                  elifConditions.push_back(elifCond);
                  bytes_done += 1 + JUMPOFFSet_Size;//JMPIFFALSE of elif
                  scope += 1;
                  before = STACK_SIZE;
                  locals.push_back(m);
                  vector<uint8_t> elifBlock = compile(ast->childs[elifBlockcounter]);
                  scope -= 1;
                  STACK_SIZE = before;
                  int32_t eliflocals = locals.back().size();
                  elifLocalSizes.push_back(eliflocals);
                  locals.pop_back();
                  bytes_done += 1 + JUMPOFFSet_Size;// JMP of elifBlock
                  elifBlocks.push_back(elifBlock);
                  elifBlocksSize += elifCond.size() + 1 + JUMPOFFSet_Size + elifBlock.size() + 1 + JUMPOFFSet_Size ;
                  if(eliflocals!=0)
                  {
                    elifBlocksSize+=4;
                    bytes_done+=4;
                  }
                  elifBlockcounter += 1;
              }
              scope += 1;
              locals.push_back(m);
              before = STACK_SIZE;
              vector<uint8_t> elseBlock = compile(ast->childs[ast->childs.size() - 2]);
              scope -= 1;
              STACK_SIZE = before;
              int32_t elseLocals = locals.back().size();
              locals.pop_back();
              program.insert(program.end(), ifcond.begin(), ifcond.end());
              program.push_back(JMPIFFALSE);
              foo = ifblock.size() + 1 + JUMPOFFSet_Size ;
              if(iflocals!=0)
                foo+=4;
              addBytes(program,foo);
              program.insert(program.end(), ifblock.begin(), ifblock.end());
              if(iflocals!=0)
              {
                program.push_back(JMPNPOPSTACK);
                foo = iflocals;
                addBytes(program,foo);
              }
              else
                program.push_back(JMP);
              foo = elifBlocksSize + elseBlock.size() ;
              if(elseLocals!=0)
                foo+=5;
              addBytes(program,foo);  
              for (size_t k = 0; k < elifBlocks.size(); k += 1)
              {

                  elifBlocksSize -= elifBlocks[k].size() + elifConditions[k].size() + 1 + (2 * JUMPOFFSet_Size) + 1;
                  if(elifLocalSizes[k]!=0)
                    elifBlocksSize -= 4;
                  program.insert(program.end(), elifConditions[k].begin(), elifConditions[k].end());
                  program.push_back(JMPIFFALSE);
                  foo = elifBlocks[k].size() + JUMPOFFSet_Size + 1;
                  if(elifLocalSizes[k]!=0)
                    foo+=4;
                  addBytes(program,foo);
                  program.insert(program.end(), elifBlocks[k].begin(), elifBlocks[k].end());
                  if(elifLocalSizes[k]!=0)
                  {

                    program.push_back(JMPNPOPSTACK);
                    foo = elifLocalSizes[k];
                    addBytes(program,foo);
                  }
                  else
                    program.push_back(JMP);
                  foo = elifBlocksSize + elseBlock.size() ;
                  if(elseLocals!=0)
                    foo+=5;
                  addBytes(program,foo);
              }

              program.insert(program.end(), elseBlock.begin(), elseBlock.end());
              if(elseLocals!=0)
              {
                program.push_back(NPOP_STACK);
                foo = elseLocals;
                addBytes(program,foo);
                bytes_done+=5;
              }
          }
          else if (ast->type == NodeType::IFELIF)
          {
            
            vector<uint8_t> ifcond = exprByteCode(ast->childs[1]->childs[0]);
            bytes_done += 1 + JUMPOFFSet_Size;//JumpIfFalse after if condition
            scope += 1;
            std::unordered_map<string,int32_t> m;
            locals.push_back(m);
            int32_t before = STACK_SIZE;
            vector<uint8_t> ifblock = compile(ast->childs[2]);
            scope -= 1;
            STACK_SIZE = before;
            int32_t iflocals = locals.back().size();
            locals.pop_back();
            bytes_done += 1 + JUMPOFFSet_Size;//JMP of if block
            if(iflocals!=0)
              bytes_done+=4;//for NPOP_STACK+ 4 byte operand
            vector<vector<uint8_t>> elifConditions;
            int32_t elifBlocksSize = 0;//total size in bytes elif blocks take including their conditions
            int32_t elifBlockcounter = 3;//third node of ast
            vector<int32_t> elifLocalSizes ;
            vector<vector<uint8_t>> elifBlocks;
            for (size_t k = 1; k < ast->childs[1]->childs.size(); k += 1)
            {
                vector<uint8_t> elifCond = exprByteCode(ast->childs[1]->childs[k]);
                elifConditions.push_back(elifCond);
                bytes_done += 1 + JUMPOFFSet_Size;//JMPIFFALSE of elif
                scope += 1;
                before = STACK_SIZE;
                locals.push_back(m);
                vector<uint8_t> elifBlock = compile(ast->childs[elifBlockcounter]);
                scope -= 1;
                STACK_SIZE = before;
                int32_t eliflocals = locals.back().size();
                elifLocalSizes.push_back(eliflocals);
                locals.pop_back();
                bytes_done += 1 + JUMPOFFSet_Size;// JMP of elifBlock
                elifBlocks.push_back(elifBlock);
                elifBlocksSize += elifCond.size() + 1 + JUMPOFFSet_Size + elifBlock.size() + 1 + JUMPOFFSet_Size ;
                if(eliflocals!=0)
                {
                  elifBlocksSize+=4;
                  bytes_done+=4;
                }
                elifBlockcounter += 1;
            }

            program.insert(program.end(), ifcond.begin(), ifcond.end());
            program.push_back(JMPIFFALSE);
            foo = ifblock.size() + 1 + JUMPOFFSet_Size ;
            if(iflocals!=0)
              foo+=4;
            addBytes(program,foo);
            program.insert(program.end(), ifblock.begin(), ifblock.end());
            if(iflocals!=0)
            {
              program.push_back(JMPNPOPSTACK);
              foo = iflocals;
              addBytes(program,foo);
            }
            else
              program.push_back(JMP);
            foo = elifBlocksSize;
            addBytes(program,foo);
            for (size_t k = 0; k < elifBlocks.size(); k += 1)
            {
                elifBlocksSize -= elifBlocks[k].size() + elifConditions[k].size() + 1 + (2 * JUMPOFFSet_Size) + 1;
                if(elifLocalSizes[k]!=0)
                  elifBlocksSize -= 4;
                program.insert(program.end(), elifConditions[k].begin(), elifConditions[k].end());
                program.push_back(JMPIFFALSE);
                foo = elifBlocks[k].size() + JUMPOFFSet_Size + 1;
                if(elifLocalSizes[k]!=0)
                  foo+=4;
                addBytes(program,foo);
                program.insert(program.end(), elifBlocks[k].begin(), elifBlocks[k].end());
                if(elifLocalSizes[k]!=0)
                {

                  program.push_back(JMPNPOPSTACK);
                  foo = elifLocalSizes[k];
                  addBytes(program,foo);
                }
                else
                  program.push_back(JMP);
                foo = elifBlocksSize ;
                addBytes(program,foo);
            }


          }
          
          else if(ast->type==NodeType::TRYCATCH)
          {
            program.push_back(ONERR_GOTO);
            bytes_done+=5;
            scope += 1;
            int32_t before = STACK_SIZE;
            std::unordered_map<string,int32_t> m;
            locals.push_back(m);
            vector<uint8_t> tryBlock = compile(ast->childs[2]);
            scope -= 1;
            STACK_SIZE = before;
            int32_t trylocals = locals.back().size();
            locals.pop_back();

            foo = bytes_done+6 +(trylocals==0 ? 0:5);
            addBytes(program,foo);
            program.insert(program.end(),tryBlock.begin(),tryBlock.end());
            if(trylocals!=0)
            {
              foo = trylocals;
              program.push_back(NPOP_STACK);
              addBytes(program,foo);
              bytes_done+=5;

            }
            program.push_back(POP_EXCEP_TARG);
            bytes_done+=6;
            scope += 1;
            before = STACK_SIZE;
            m.emplace(ast->childs[1]->val,STACK_SIZE);
            STACK_SIZE+=1;
            locals.push_back(m);
            vector<uint8_t> catchBlock = compile(ast->childs[3]);
            int32_t catchlocals = locals.back().size();
            scope-=1;
            foo = catchBlock.size()+(catchlocals==1 ? 1 : 5);
            program.push_back(JMP);
            addBytes(program,foo);
            program.insert(program.end(),catchBlock.begin(),catchBlock.end());
            STACK_SIZE = before;

            if(catchlocals>1)
            {
            foo = catchlocals;
            program.push_back(NPOP_STACK);
            addBytes(program,foo);
            bytes_done+=5;
            }
            {
              program.push_back(POP_STACK);
              bytes_done+=1;
            }
            locals.pop_back();

          }
          else if (ast->type==NodeType::FUNC || (isGen = ast->type == NodeType::CORO))
          {
            int32_t selfIdx= 0;
            if(infunc)
            {
              line_num = atoi(ast->childs[0]->val.c_str());
              compileError("SyntaxError","Error function within function not allowed!");
            }
            const string& name = ast->childs[1]->val;
            if(compileAllFuncs || std::find(fnReferenced->begin(),fnReferenced->end(),name)!=fnReferenced->end() || inclass)
            {
                if(funcs.find(name)!=funcs.end() && !inclass)
                  compileError("NameError","Error a builtin function with same name already exists!");
                if(scope==0 && globals.find(name)!=globals.end())
                  compileError("NameError","Error redeclaration of name "+name);
                else if(scope!=0 && locals.back().find(name)!=locals.back().end())
                  compileError("NameError","Error redeclaration of name "+name);
                if(!inclass)
                {
                  if(scope==0)
                    globals.emplace(name,STACK_SIZE);
                  else
                    locals.back().emplace(name,STACK_SIZE);
                  STACK_SIZE+=1;
                }
                unordered_map<string,int32_t> C;
                uint8_t def_param =0;
                vector<uint8_t> expr;
                int32_t before = STACK_SIZE;
                STACK_SIZE = 0;
                if(inclass)
                {
                  locals.back().emplace("self",STACK_SIZE);
                  selfIdx = STACK_SIZE;
                  if(name=="__construct__")
                    inConstructor = true;
                  STACK_SIZE+=1;
                }
                for (size_t k =0; k<ast->childs[2]->childs.size(); k += 1)
                {
                  string n = ast->childs[2]->childs[k]->val;
                  if(ast->childs[2]->childs[k]->childs.size()!=0)
                  {
                    if(isGen)
                      compileError("SyntaxError","Error default parameters not supported for couroutines");
                    expr = exprByteCode(ast->childs[2]->childs[k]->childs[0]);
                    program.insert(program.end(),expr.begin(),expr.end());
                    def_param +=1;
                  }
                  C.emplace(n,STACK_SIZE);
                  STACK_SIZE+=1;
                }
                if(isGen)
                  program.push_back(LOAD_CO);
                else
                  program.push_back(LOAD_FUNC);
                foo = bytes_done+2+JUMPOFFSet_Size+JUMPOFFSet_Size+JUMPOFFSet_Size+1 +((isGen) ? 0 : 1);
                addBytes(program,foo);
                foo = addToVMStringTable(name);
                addBytes(program,foo);
                if(inclass)
                    program.push_back(ast->childs[2]->childs.size()+1);
                else
                  program.push_back(ast->childs[2]->childs.size());
                //Push number of optional parameters
                if(!isGen)
                {
                  program.push_back(def_param);
                  bytes_done++;
                }
                bytes_done+=10;

                /////////
                int32_t I = program.size();
                program.push_back(JMP);
                addBytes(program,0);
                bytes_done += 1 + JUMPOFFSet_Size ;
                
                locals.push_back(C);
                scope += 1;
                infunc = true;
                inGen = isGen;
                vector<uint8_t> funcBody = compile(ast->childs[3]);
                infunc = false;
                scope -= 1;
                inConstructor = false;
                inGen = false;
                locals.pop_back();
                STACK_SIZE = before;
                if(name!="__construct__")
                {
                  if (funcBody.size() == 0)
                  {
                        foo = 0;
                        funcBody.push_back(LOAD_CONST);
                        addBytes(funcBody,foo);
                        if(isGen)
                          funcBody.push_back(CO_STOP);
                        else
                          funcBody.push_back(RETURN);
                        bytes_done +=6;
                  }
                  else
                  {
                    if (funcBody[funcBody.size() - 1] != RETURN)
                    {
                        foo = 0;
                        funcBody.push_back(LOAD_CONST);
                        addBytes(funcBody,foo);
                        if(isGen)
                          funcBody.push_back(CO_STOP);
                        else
                          funcBody.push_back(RETURN);
                        bytes_done +=6;
                    }
                  }
                }
                else
                {
                  funcBody.push_back(LOAD);
                  funcBody.push_back('v');
                  foo = selfIdx;
                  addBytes(funcBody,foo);
                  funcBody.push_back(RETURN);
                  bytes_done+=7;
                }
                //program.push_back(JMP);
                foo =  funcBody.size();
                memcpy(&program[I+1],&foo,sizeof(int));
                //addBytes(program,foo);

                program.insert(program.end(), funcBody.begin(), funcBody.end());
            }
            else
            {
             // printf("not compiling function %s because it is not fnReferenced\n",name.c_str());
            }
            isGen = false;
          }
          else if(ast->type==NodeType::CLASS || ast->type==NodeType::EXTCLASS)
          {
            if(scope!=0)
            {
              compileError("SyntaxError","Class declaration must be at global scope!");
            }
            bool extendedClass = (ast->type == NodeType::EXTCLASS);
            string name = ast->childs[1]->val;
            size_t C = line_num;
            if(globals.find(name)!=globals.end())
                compileError("NameError","Redeclaration of name "+name);
            globals.emplace(name,STACK_SIZE);
            
            STACK_SIZE+=1;
            vector<string> names = scanClass(ast->childs[2]);
            if(extendedClass)
            {
              vector<uint8_t> baseClass = exprByteCode(ast->childs[ast->childs.size()-2]);
              program.insert(program.end(),baseClass.begin(),baseClass.end());
            }
            for(auto e: names)
            {
              if(e=="super" || e=="self")
              {
                compileError("NameError","Error class not allowed to have members named \"super\" or \"self\"");
              }
              foo = addToVMStringTable(e);
              program.push_back(LOAD_STR);
              addBytes(program,foo);
              bytes_done+=5;
            }
            std::unordered_map<string,int32_t> M;
            locals.push_back(M);
            int32_t before = STACK_SIZE;
            scope+=1;
            inclass = true;
            className = name;
            classMemb = names;
            vector<uint8_t> block = compile(ast->childs[2]);
            inclass = false;
            scope-=1;

            locals.pop_back();
            int32_t totalMembers = names.size();
            STACK_SIZE = before;

            program.insert(program.end(),block.begin(),block.end());
            if(extendedClass)
            {
              program.push_back(BUILD_DERIVED_CLASS);
              addLnTableEntry(bytes_done);
            }
            else
              program.push_back(BUILD_CLASS);

            addBytes(program,totalMembers);
            foo = addToVMStringTable(name);
            addBytes(program,foo);
            bytes_done+=9;


          }
          else if (ast->type == NodeType::RETURN)
          {
              if(inConstructor)
                compileError("SyntaxError","Error class constructors should not return anything!");
              
              vector<uint8_t> val = exprByteCode(ast->childs[1]);
              program.insert(program.end(), val.begin(), val.end());
              addLnTableEntry(bytes_done);
              if(inGen)
                program.push_back(CO_STOP);
              else
                program.push_back(RETURN);
              bytes_done += 1;
          }
          else if (ast->type == NodeType::THROW)
          {   
              vector<uint8_t> val = exprByteCode(ast->childs[1]);
              program.insert(program.end(), val.begin(), val.end());
              addLnTableEntry(bytes_done);
              program.push_back(THROW);
              bytes_done += 1;
          }
          else if (ast->type == NodeType::YIELD)
          {
              if(inConstructor)
                compileError("SyntaxError","Error class constructor can not be generators!");
              
              vector<uint8_t> val = exprByteCode(ast->childs[1]);
              program.insert(program.end(), val.begin(), val.end());              
              addLnTableEntry(bytes_done);
              program.push_back(YIELD);
              bytes_done += 1;
          }
          else if (ast->type == NodeType::BREAK)
          {

              program.push_back(BREAK);
              foo = breakTargets.back();
              addBytes(program,foo);
              foo = 0;
              for(int32_t i = locals.size()-1;i>=indexOfLastWhileLocals.back();i-=1)
                foo += locals[i].size();
              addBytes(program,foo);
              bytes_done += 9;
          }
          else if(ast->type==NodeType::gc)
          {
            program.push_back(GC);
            bytes_done+=1;
          }
          else if (ast->type == NodeType::CONTINUE)
          {

              program.push_back(CONT);
              foo = contTargets.back();
              addBytes(program,foo);

          //    foo = scope - scopeOfLastWhile.back();
              foo = 0;
              for(int32_t i = locals.size()-1;i>=indexOfLastWhileLocals.back();i-=1)
                foo += locals[i].size();
              addBytes(program,foo);
              bytes_done += 9;
          }
          else if (ast->type == NodeType::call)
          {
              const string& name = ast->childs[1]->val;
              bool udf = (funcs.find(name) == funcs.end());  
              if (udf)
              {
                  vector<uint8_t> callExpr = exprByteCode(ast);
                  program.insert(program.end(),callExpr.begin(),callExpr.end());
                  program.push_back(POP_STACK);
                  bytes_done+=1;
              }
              else
              {
                  for (size_t k = 0; k < ast->childs[2]->childs.size(); k += 1)
                  {
                      vector<uint8_t> val = exprByteCode(ast->childs[2]->childs[k]);
                      program.insert(program.end(), val.begin(), val.end());
                  }
                  addLnTableEntry(bytes_done);
                  program.push_back(CALL);
                  bytes_done += 1;
                  size_t index = 0;
                  bool add = true;
                  for(index = 0;index < vm.builtin.size();index+=1)
                  {
                    if(vm.builtin[index]==funcs[name])
                    {
                      add = false;
                      break;
                    }
                  }
                  if(add)
                  {
                    vm.builtin.push_back(funcs[name]);
                    foo = vm.builtin.size()-1;
                  }
                  else
                    foo = index;
                  addBytes(program,foo);
                  bytes_done += 5;
                  program.push_back((char)ast->childs[2]->childs.size());

              }
          }
          else if(ast->type == NodeType::file)
          {
            if(infunc)
              compileError("SyntaxError","Error file import inside function!");
            if(inclass)
              compileError("SyntaxError","Error file import inside class!");
              
            string C = filename;
            filename = ast->childs[1]->val;
            short K = fileTOP;
            fileTOP = std::find(files->begin(),files->end(),filename) - files->begin();
            bool a = infunc;
            bool b = inclass;
            infunc = false;
            inclass = false;
            vector<uint8_t> fByteCode = compile(ast->childs[2]);
            infunc = a;
            inclass = b;
            program.insert(program.end(),fByteCode.begin(),fByteCode.end());
            filename = C;
            fileTOP = K;
          }
          else
          {
              printf("SyntaxError in file %s\n%s\nThe line does not seem to be a meaningful statement!\n", filename.c_str(), NodeTypeNames[(int)ast->type]);
              exit(0);
          }
          ast = ast->childs.back();
        //  printf("ast = %s\n",NodeTypeNames[(int)ast->type]);
      }
      return program;

  }
  void optimizeJMPS(vector<uint8_t>& bytecode)
  {
        for(auto e: andJMPS)
    {
      int offset;
      memcpy(&offset,&bytecode[e+1],4);
      int k = e+5+offset;
      int newoffset;
      while(k < bytes_done && bytecode[k] == JMPIFFALSENOPOP)
      {
        memcpy(&newoffset,&bytecode[k+1],4);
        offset+=newoffset+5;
        k = k+5+newoffset;
      }
      memcpy(&bytecode[e+1],&offset,4);
    }
    //optimize short circuit jumps for or
    for(auto e: orJMPS)
    {
      int offset;
      memcpy(&offset,&bytecode[e+1],4);
      int k = e+5+offset;
      int newoffset;
      while(k < bytes_done && bytecode[k] == NOPOPJMPIF)
      {
        memcpy(&newoffset,&bytecode[k+1],4);
        offset+=newoffset+5;
        k = k+5+newoffset;
      }
      memcpy(&bytecode[e+1],&offset,4);
    }
  }
  void reduceStackTo(int size)//for REPL
  {
    while((int)STACK_SIZE > size)
    {
      int lastidx = (int)STACK_SIZE-1;
      int firstidx = (int)size;
      for(auto it=globals.begin();it!=globals.end();)
      {
        if((*it).second>=firstidx  && (*it).second <= lastidx)
        {
          it = globals.erase(it);
          STACK_SIZE-=1;
        }
        else
          ++it;
      }
    }
  }
  Klass* makeErrKlass(string name,Klass* error)
  {
    Klass* k = allocKlass();
    k->name=name;
    k->members = error->members;
    k->privateMembers = error->privateMembers;
    globals.emplace(name,STACK_SIZE++);
    vm.STACK.push_back(PObjFromKlass(k));
    return k;
  }
  vector<uint8_t> compileProgram(Node* ast,int32_t argc,const char* argv[],vector<uint8_t> prev = {},bool compileNonRefFns = false,bool popGlobals = true)//compiles as a complete program adds NPOP_STACK and OP_EXIT
  {
    //If prev vector is empty then this program will be compiled as an independent new one
    //otherwise this program will be an addon to the previous one
    //new = prev +curr

    bytes_done = prev.size();
    compileAllFuncs = compileNonRefFns;
    line_num = 1;
    andJMPS.clear();
    orJMPS.clear();
    vector<uint8_t> bytecode;
    if(prev.size() == 0)
    {
      STACK_SIZE = 3;
      globals.clear();
      locals.clear();
      scope = 0;
      prefixes.clear();
      globals.emplace("argv",0);
      globals.emplace("stdin",1);
      globals.emplace("stdout",2);
      int32_t k = 2;
      PltList l;
      PltObject elem;
      elem.type = 's';
      while (k < argc)
      {
          //elem.s = argv[k];
          string* p = allocString();
          *p = argv[k];
          l.push_back(PObjFromStrPtr(p));
          k += 1;
      }
      PltObject A;
      PltList* p = allocList();
      *p = l;
      vm.STACK.push_back(PObjFromList(p));
      FileObject* STDIN = allocFileObject();
      STDIN->open = true;
      STDIN ->fp = stdin;
  
      FileObject* STDOUT = allocFileObject();
      STDOUT->open = true;
      STDOUT ->fp = stdout;
      
      vm.STACK.push_back(PObjFromFile(STDIN));
      vm.STACK.push_back(PObjFromFile(STDOUT));
      
      addToVMStringTable("msg");
      PltObject nil;
      Error = allocKlass();
      Error->name = "Error";
      Error->members.emplace("msg",nil);
      globals.emplace("Error",3);
      addToVMStringTable("__construct__");
      FunObject* fun = allocFunObject();
      fun->name = "__construct__";
      fun->args = 2;
      fun->i = 5;
      fun->klass = NULL;
      vm.important.push_back((void*)fun);

      bytecode.push_back(JMP);
      addBytes(bytecode,21);
      bytecode.push_back(LOAD_LOCAL);
      addBytes(bytecode,0);
      bytecode.push_back(LOAD_LOCAL);
      addBytes(bytecode,1);
      bytecode.push_back(ASSIGNMEMB);
      addBytes(bytecode,0);
      bytecode.push_back(LOAD_LOCAL);
      addBytes(bytecode,0);
      bytecode.push_back(RETURN);

      bytes_done+=26;
      PltObject construct;
      construct.type = PLT_FUNC;
      construct.ptr = (void*)fun;
      Error->members.emplace("__construct__",construct);
      vm.STACK.push_back(PObjFromKlass(Error));
      STACK_SIZE+=1;
      TypeError = makeErrKlass("TypeError",Error);
      ValueError = makeErrKlass("ValueError",Error);
      MathError  = makeErrKlass("MathError",Error);
      NameError = makeErrKlass("NameError",Error);
      IndexError = makeErrKlass("IndexError",Error);
      ArgumentError = makeErrKlass("ArgumentError",Error);
      FileIOError = makeErrKlass("FileIOError",Error);
      KeyError = makeErrKlass("KeyError",Error);
      OverflowError = makeErrKlass("OverflowError",Error);
      FileOpenError = makeErrKlass("FileOpenError",Error);
      FileSeekError  = makeErrKlass("FileSeekError",Error);
      ImportError = makeErrKlass("ImportError",Error);
      ThrowError = makeErrKlass("ThrowError",Error);
      MaxRecursionError =makeErrKlass("MaxRecursionError",Error);
      AccessError = makeErrKlass("AccessError",Error);
    }
    infunc = false;
    inclass = false;
    auto bt = compile(ast);
    bytecode.insert(bytecode.end(),bt.begin(),bt.end());
    if(globals.size()!=0 && popGlobals)
    {
      bytecode.push_back(NPOP_STACK);
      foo = globals.size();
      addBytes(bytecode,foo);
      bytes_done+=5;
    }
    bytecode.push_back(OP_EXIT);
    bytes_done+=1;
    bytecode.insert(bytecode.begin(),prev.begin(),prev.end());
    if (bytes_done != bytecode.size())
    {
        printf("Plutonium encountered an internal error.\nError Code: 10\n");
       // printf("%ld   /  %ld  bytes done\n",bytes_done,bytecode.size());
        exit(0);
    }
    //final phase
    //optimize short circuit jumps for and 
    optimizeJMPS(bytecode);
    return bytecode;
  }
};
#endif