#include "llvmruby.h"

extern VALUE cLLVMBasicBlock;
extern VALUE cLLVMBuilder;

extern "C" {
VALUE 
llvm_basic_block_wrap(BasicBlock* bb) { 
  return Data_Wrap_Struct(cLLVMBasicBlock, NULL, NULL, bb); 
}

VALUE 
llvm_basic_block_builder(VALUE self) {
  BasicBlock* bb;
  Data_Get_Struct(self, BasicBlock, bb);
  IRBuilder<> *builder = new IRBuilder<>(bb);
  return Data_Wrap_Struct(cLLVMBuilder, NULL, NULL, builder);
}

#define DATA_GET_BUILDER IRBuilder<> *builder; Data_Get_Struct(self, IRBuilder<>, builder);
#define DATA_GET_BLOCK   BasicBlock *bb; Data_Get_Struct(rbb, BasicBlock, bb);

VALUE
llvm_builder_set_insert_point(VALUE self, VALUE rbb) {
  DATA_GET_BUILDER
  DATA_GET_BLOCK 
  builder->SetInsertPoint(bb);
  return self;
}

VALUE 
llvm_builder_bin_op(VALUE self, VALUE rbin_op, VALUE rv1, VALUE rv2) {
  DATA_GET_BUILDER

  Instruction::BinaryOps bin_op = (Instruction::BinaryOps)FIX2INT(rbin_op);
  
  Value *v1, *v2; 
  Data_Get_Struct(rv1, Value, v1);
  Data_Get_Struct(rv2, Value, v2);
  Value *res = builder->CreateBinOp(bin_op, v1, v2);
  return llvm_value_wrap(res);
}

VALUE
llvm_builder_create_phi(VALUE self, VALUE type) {
  DATA_GET_BUILDER
  PHINode *v = builder->CreatePHI(LLVM_TYPE(type)); 
  return Data_Wrap_Struct(cLLVMPhi, NULL, NULL, v);
}

VALUE
llvm_phi_add_incoming(VALUE self, VALUE val, VALUE block) {
  PHINode *phi = LLVM_PHI(self);
  phi->addIncoming(LLVM_VAL(val), LLVM_BASIC_BLOCK(block));
  return self;
}

VALUE 
llvm_builder_create_return(VALUE self, VALUE rv) {
  DATA_GET_BUILDER

  Value *v;
  Data_Get_Struct(rv, Value, v);
  return llvm_value_wrap(builder->CreateRet(v));
}

VALUE 
llvm_builder_create_br(VALUE self, VALUE rblock) {
  DATA_GET_BUILDER

  BasicBlock *bb;
  Data_Get_Struct(rblock, BasicBlock, bb);
  return llvm_value_wrap(builder->CreateBr(bb)); 
}  

VALUE 
llvm_builder_create_cond_br(VALUE self, VALUE rcond, VALUE rtrue_block, VALUE rfalse_block) {
  DATA_GET_BUILDER

  Value *cond;
  Data_Get_Struct(rcond, Value, cond);

  BasicBlock *true_block, *false_block;
  Data_Get_Struct(rtrue_block, BasicBlock, true_block);
  Data_Get_Struct(rfalse_block, BasicBlock, false_block);

  return llvm_value_wrap(builder->CreateCondBr(cond, true_block, false_block));
}
  
VALUE 
llvm_builder_create_alloca(VALUE self, VALUE rtype, VALUE rsize) {
  DATA_GET_BUILDER

  const Type* type;
  Data_Get_Struct(rtype, Type, type);

  Value *size = ConstantInt::get(Type::Int32Ty, FIX2INT(rsize));
  Value *v = builder->CreateAlloca(type, size);
  return llvm_value_wrap(v);
}

VALUE
llvm_builder_create_load(VALUE self, VALUE rptr) {
  DATA_GET_BUILDER

  Value *ptr;
  Data_Get_Struct(rptr, Value, ptr);
  return llvm_value_wrap(builder->CreateLoad(ptr));
}

VALUE
llvm_builder_create_store(VALUE self, VALUE rv, VALUE rptr) {
  DATA_GET_BUILDER

  Value *v, *ptr;
  Data_Get_Struct(rv, Value, v);
  Data_Get_Struct(rptr, Value, ptr);
  return llvm_value_wrap(builder->CreateStore(v, ptr));
}

VALUE
llvm_builder_create_icmpeq(VALUE self, VALUE rlhs, VALUE rrhs) {
  DATA_GET_BUILDER

  Value *lhs, *rhs;
  Data_Get_Struct(rlhs, Value, lhs);
  Data_Get_Struct(rrhs, Value, rhs);
  return llvm_value_wrap(builder->CreateICmpEQ(lhs, rhs));
}

VALUE
llvm_builder_create_icmpult(VALUE self, VALUE rlhs, VALUE rrhs) {
  DATA_GET_BUILDER

  Value *lhs, *rhs;
  Data_Get_Struct(rlhs, Value, lhs);
  Data_Get_Struct(rrhs, Value, rhs);
  return llvm_value_wrap(builder->CreateICmpULT(lhs, rhs));
}

VALUE
llvm_builder_create_icmp(VALUE self, VALUE pred, VALUE lhs, VALUE rhs) {
  DATA_GET_BUILDER

  CmpInst::Predicate p = (CmpInst::Predicate)FIX2INT(pred);
  Value *v = builder->CreateICmp(p, LLVM_VAL(rhs), LLVM_VAL(lhs));
  return llvm_value_wrap(v);
}

VALUE
llvm_builder_create_gep(VALUE self, VALUE rptr, VALUE ridx) {
  DATA_GET_BUILDER

  Value *ptr, *idx;
  Data_Get_Struct(rptr, Value, ptr);
  Data_Get_Struct(ridx, Value, idx);
  return llvm_value_wrap(builder->CreateGEP(ptr, idx));
}

VALUE
llvm_builder_create_struct_gep(VALUE self, VALUE rptr, VALUE ridx) {
  DATA_GET_BUILDER 

  Value *ptr; 
  Data_Get_Struct(rptr, Value, ptr);
  return llvm_value_wrap(builder->CreateStructGEP(ptr, FIX2INT(ridx)));
}

VALUE
llvm_builder_create_int_to_ptr(VALUE self, VALUE ri, VALUE rtype) {
  DATA_GET_BUILDER

  Value *i;
  Data_Get_Struct(ri, Value, i);
 
  const Type* type; 
  Data_Get_Struct(rtype, Type, type);

  return llvm_value_wrap(builder->CreateIntToPtr(i, type));
}

VALUE llvm_builder_create_int_cast(VALUE self, VALUE i, VALUE type, VALUE sign) {
  DATA_GET_BUILDER
  bool isSigned = (sign != Qnil && sign != Qfalse);
  return llvm_value_wrap(builder->CreateIntCast(LLVM_VAL(i), LLVM_TYPE(type), isSigned));
}

VALUE
llvm_builder_create_call(int argc, VALUE* argv, VALUE self) {
  DATA_GET_BUILDER

  Function *callee = LLVM_FUNCTION(argv[0]);
  int num_args = argc-1;
  Value** args = (Value**)alloca(num_args*sizeof(Value*));
  for(int i = 0; i < num_args; ++i) {
    args[i] = LLVM_VAL(argv[i+1]); 
  }
  return llvm_value_wrap(builder->CreateCall(callee, args, args+num_args));
}

VALUE
llvm_builder_get_global(VALUE self) {
  GlobalVariable *g = new GlobalVariable(Type::Int64Ty, false, GlobalValue::ExternalLinkage, 0, "shakalaka");
  return llvm_value_wrap(g);
}
}
