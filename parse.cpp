#include <iostream>
#include <vector>

#include "llvm/Support/CommandLine.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Parse/Parser.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace std;

static llvm::cl::OptionCategory MyToolCategory("");

class myASTVisitor : public RecursiveASTVisitor<myASTVisitor> {
  ASTContext *Context;
public:
  myASTVisitor (ASTContext *Context) : Context(Context) {}

  bool VisitTypedefDecl (TypedefDecl *D) {
    if(qtSourceP(D)) {
      cout << "(:typedef \"" << D->getUnderlyingType().getAsString() << "\" \""
           << D->getQualifiedNameAsString() << "\")\n";
    }
    return true;
  }

  bool VisitCXXRecordDecl (CXXRecordDecl *D) {
    if(qtSourceP(D) && D->isCompleteDefinition() && D->isClass()) {
      cout << "(:class \"" << D->getQualifiedNameAsString() << "\"\n";
      cout << " :constructors (";
      for (CXXRecordDecl::ctor_iterator iter = D->ctor_begin(); 
           iter != D->ctor_end(); iter++) {
        printFunParams(llvm::dyn_cast<CXXConstructorDecl>(*iter));

      }
      cout << ")\n";

      {
        CXXRecordDecl::method_iterator iter = D->method_begin();
        if (iter != D->method_end()) {
          cout << " :methods\n  (";
          for (;;) {
            printMethod(llvm::dyn_cast<CXXMethodDecl>(*iter));
            if (++iter == D->method_end())
              break;
            cout << ")\n   ";
          }
          cout << ")\n";
        }
      }

      cout << ")\n";
    }
    return true;
  }

private:

  void printMethod(CXXMethodDecl *D) {
    cout << '(';
    cout << '"' << D->getNameAsString() << "\" \"" << D->getReturnType().getAsString() << "\" ";
    printFunParams(D);
    cout << ')';
  }

  void printFunParams(FunctionDecl *D) {
    FunctionDecl::param_iterator iter = D->param_begin();
    cout << '(';
    if (iter != D->param_end()) {
      for(;;) {
        ParmVarDecl *decl = llvm::dyn_cast<ParmVarDecl>(*iter);
      
        cout << "(\"" << decl->getType().getAsString() << "\" \"" 
             << decl->getQualifiedNameAsString() << '"';
        if (decl->hasDefaultArg()) {
          LangOptions LangOpts;
          LangOpts.CPlusPlus = true;
          PrintingPolicy Policy(LangOpts);
          string str;
          llvm::raw_string_ostream s(str);
          decl->getDefaultArg()->printPretty(s, 0, Policy);
          cout << " :default \"" << s.str() << '"';
        }
        cout << ')';
        
        if (++iter == D->param_end())
          break;
        cout << ' ';
      }
    }
    cout << ')';
  }
  

  bool qtSourceP(SourceLocation loc) {
    SourceManager &SM = Context->getSourceManager();
    if (!loc.isFileID()) {
      loc = SM.getExpansionLoc(loc);
    }
    string file = SM.getFilename(loc).str();

    return file.find("/qt5/Qt") != string::npos;
  }

  bool qtSourceP(DeclaratorDecl *decl) {
    return qtSourceP(decl->getLocStart());
  }

  bool qtSourceP(TypeDecl *decl) {
    return qtSourceP(decl->getLocStart());
  }

};

class MyASTConsumer : public clang::ASTConsumer
{
  myASTVisitor Visitor;
public:
  MyASTConsumer(ASTContext *Context) : Visitor(Context) { }

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

class myAction : public clang::ASTFrontendAction {
 public:
  myAction() {}

  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(new MyASTConsumer(&Compiler.getASTContext()));
  }
};

int main(int argc, const char **argv)
{
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  Tool.run(newFrontendActionFactory<myAction>().get());
  return 0;
}
