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
      std::cout << D->getQualifiedNameAsString() << endl;
    }
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *D) {
    if(qtSourceP(D)) {
      std::cout << llvm::dyn_cast<DeclContext>(D)->getDeclKindName() << ' ';
      std::cout << D->getReturnType().getAsString() << ' ' ;

      std::cout << D->getQualifiedNameAsString() << '(';
      FunctionDecl::param_iterator iter;
      for( iter = D->param_begin(); iter != D->param_end(); iter++) {
        ParmVarDecl *decl = llvm::dyn_cast<clang::ParmVarDecl>(*iter);

        std::cout << ' ' << decl->getType().getAsString() << ' ' << decl->getQualifiedNameAsString() << ',';

      }
      std::cout << ')' << std::endl;
    }

    return true;
  }

private:

  

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
