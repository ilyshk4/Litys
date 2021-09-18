#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <chrono>
#include "Lexer.h"
#include "Parser.h"
#include "Object.h"

using namespace std;

int FuncPrint(VM* vm) {
    for (int i = 0; i < vm->GetParametersCount(); i++)
        cout << vm->GetParameter(i).ToString() << ' ';
    cout << endl;
    return 0;
}

int FuncInput(VM* vm) {
    std::string str;
    std::getline(std::cin, str);
    auto obj = new StringObject(str.c_str());
    vm->Push(obj);
    vm->NewObject(obj);
    return 1;
}

int FuncMathSin(VM* vm) {
    vm->Push(sin(vm->GetParameter(0).as.number));
    return 1;
}

int FuncMathPow(VM* vm) {
    vm->Push(pow(vm->GetParameter(0).as.number, vm->GetParameter(1).as.number));
    return 1;
}

int FuncNow(VM* vm) {
    vm->Push((double)chrono::high_resolution_clock::now().time_since_epoch().count());
    return 1;
}

int FuncString(VM* vm) {
    string r;
    for (int i = 0; i < vm->GetParametersCount(); i++)
        r += vm->GetParameter(i).ToString();
    auto obj = new StringObject(r.c_str());
    vm->NewObject(obj);
    vm->Push(obj);
    return 1;
}

int FuncInt(VM* vm) {
    vm->Push((int)round(stod(vm->GetParameter(0).ToString().c_str())));
    return 1;
}

int FuncNumber(VM* vm) {
    vm->Push(stod(vm->GetParameter(0).ToString().c_str()));
    return 1;
}

int FuncCG(VM* vm) {
    vm->CollectGarbage();
    return 1;
}

int factorial(int x) {
    if (x == 1) {
        return 1;
    }
    else {
        return x * factorial(x - 1);
    }
}

int fib(int n) {
    if (n < 2)
        return 2;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char* argv[])
{
    ifstream t(argv[1]);
    string str((std::istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    string error;

    Lexer lexer(str);
    lexer.Tokenize();
    if (!lexer.Error(error))
    {
        int j = 0;
        // cout << "TOKENS:" << endl;
        // for (auto i : lexer.Get())
        // {
        //     cout << j << " " << TokenTypeName(i.type) << " " << (i.value.ToString()) << endl;
        //     j++;
        // }
        
        Parser parser(lexer.Get());
        Node* result = parser.Parse();
        // cout << endl << "AST:" << endl;
        // cout << result->ToString() << endl;
        
        Assembly assembly;
        result->Compile(assembly);

        // cout << endl << "CODE:" << endl;
        // int k = 0;
        // for (int j = 0; j < assembly.operations.size(); j++) {
        //     auto i = assembly.operations[j];
        //     if (i.code == OP_POP_FRAME)
        //         k--;
        //     cout << Indent(k) << j << " " << OperationCodeName(i.code) << " " << (i.value.ToString()) << endl;
        //     if (i.code == OP_ADD_FRAME)
        //         k++;
        // }
            
        VM vm(assembly);

        vm.Add("print", new CFunctionObject(FuncPrint));
        vm.Add("input", new CFunctionObject(FuncInput));
        vm.Add("sin", new CFunctionObject(FuncMathSin));
        vm.Add("pow", new CFunctionObject(FuncMathPow));
        vm.Add("now", new CFunctionObject(FuncNow));
        vm.Add("string", new CFunctionObject(FuncString));
        vm.Add("int", new CFunctionObject(FuncInt));
        vm.Add("number", new CFunctionObject(FuncNumber));

        vm.Add("collect_garbage", new CFunctionObject(FuncCG));

        vm.Run();

        delete result;
    }
    else
        cout << error << endl;
}