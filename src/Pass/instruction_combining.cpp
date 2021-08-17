#include "../../include/Pass/instruction_combining.h"

static bool isSameOpn(ir::Opn opn1, ir::Opn opn2){
    if (opn1.type_ == opn2.type_ && opn1.type_ == ir::Opn::Type::Var){
        if (opn1.name_ == opn2.name_ && opn1.scope_id_ == opn2.scope_id_){
            return true;
        }
        return false;
    }
    else if (opn1.type_ == opn2.type_ && opn1.type_ == ir::Opn::Type::Array){
        //先不考虑数组的情况，这里什么也没有
    }else if(opn1.type_ == opn2.type_ && opn1.type_ == ir::Opn::Type::Imm){
        return opn1.imm_num_ == opn2.imm_num_;
    }

    return false;
}

bool isImm(ir::Opn opn){
    return opn.type_ == ir::Opn::Type::Imm;
}

void Instruction_Combining::Run(){
    auto m = dynamic_cast<IRModule *>(*m_);

    for (auto func : m->func_list_){
        for (auto bb : func->bb_list_){
            for (auto iter = bb->ir_list_.begin(); iter != bb->ir_list_.end() - 1; ++iter){
                auto ir = *iter;
                auto ir_next = *(iter + 1);

                if (ir::IR::OpKind::ADD == ir->op_){
                    if (ir::IR::OpKind::ADD == ir_next->op_){
                        if (isSameOpn(ir->res_, ir_next->opn1_)){
                            if (isImm(ir_next->opn2_)){
                                if(isImm(ir->opn1_)){
                                    int opn_num = ir_next->opn2_.imm_num_ + ir->opn1_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::ADD, new_opn, ir->opn2_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }else if(isImm(ir->opn2_)){
                                    int opn_num = ir_next->opn2_.imm_num_ + ir->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::ADD, new_opn, ir->opn1_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }
                            }
                        }
                        else if (isSameOpn(ir->res_, ir_next->opn2_)){
                            if (isImm(ir_next->opn1_)){
                                if(isImm(ir->opn1_)){
                                    int opn_num = ir_next->opn1_.imm_num_ + ir->opn1_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::ADD, new_opn, ir->opn2_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }else if(isImm(ir->opn2_)){
                                    int opn_num = ir_next->opn1_.imm_num_ + ir->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::ADD, new_opn, ir->opn1_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }
                            }
                        }
                    }
                    else if (ir::IR::OpKind::SUB == ir_next->op_){
                        if(isSameOpn(ir->res_, ir_next->opn1_)){
                            if(isSameOpn(ir_next->opn2_, ir->opn1_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn2_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }else if(isSameOpn(ir_next->opn2_, ir->opn2_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn1_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }
                        }else if(isSameOpn(ir->res_, ir_next->opn2_)){
                            if(isSameOpn(ir_next->opn2_, ir->opn1_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::NEG, ir->opn2_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }else if(isSameOpn(ir_next->opn2_, ir->opn1_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::NEG, ir->opn1_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }                            
                        }
                    }
                }
                else if (ir::IR::OpKind::SUB == ir->op_){
                    if(ir::IR::OpKind::SUB == ir_next->op_){
                        if (isSameOpn(ir->res_, ir_next->opn1_)){
                            if (isImm(ir_next->opn2_)){
                                if(isImm(ir->opn1_)){
                                    int opn_num = ir->opn1_.imm_num_ - ir_next->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::SUB, new_opn, ir->opn2_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }else if(isImm(ir->opn2_)){
                                    int opn_num = ir_next->opn2_.imm_num_ + ir->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::SUB, ir->opn1_, new_opn, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }
                            }
                        }
                    }else if(ir::IR::OpKind::ADD == ir_next->op_){
                        if(isSameOpn(ir->res_, ir_next->opn1_)){
                            if(isSameOpn(ir->opn2_, ir_next->opn2_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn1_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }
                        }else if(isSameOpn(ir->res_, ir_next->opn2_)){
                            if(isSameOpn(ir->opn2_, ir_next->opn1_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn1_, ir_next->res_);

                                *(iter + 1) = new_ir;                                
                            }
                        }
                    }
                }
                else if (ir::IR::OpKind::MUL == ir->op_){
                    if (ir::IR::OpKind::MUL == ir_next->op_){
                        if (isSameOpn(ir->res_, ir_next->opn1_)){
                            if (isImm(ir_next->opn2_)){
                                if(isImm(ir->opn1_)){
                                    int opn_num = ir_next->opn2_.imm_num_ * ir->opn1_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::MUL, new_opn, ir->opn2_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }else if(isImm(ir->opn2_)){
                                    int opn_num = ir_next->opn2_.imm_num_ * ir->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::MUL, new_opn, ir->opn1_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }
                            }
                        }
                        else if (isSameOpn(ir->res_, ir_next->opn2_)){
                            if (isImm(ir_next->opn1_)){
                                if(isImm(ir->opn1_)){
                                    int opn_num = ir_next->opn1_.imm_num_ * ir->opn1_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::MUL, new_opn, ir->opn2_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }else if(isImm(ir->opn2_)){
                                    int opn_num = ir_next->opn1_.imm_num_ * ir->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::MUL, new_opn, ir->opn1_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }
                            }
                        }
                    }else if(ir::IR::OpKind::DIV == ir->op_){
                        if(isSameOpn(ir->res_, ir_next->opn1_)){
                            if(isSameOpn(ir_next->opn2_, ir->opn1_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn2_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }else if(isSameOpn(ir_next->opn2_, ir->opn2_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn1_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }
                        // }else if(isSameOpn(ir->res_, ir_next->opn2_)){
                        //     if(isSameOpn(ir_next->opn2_, ir->opn1_)){
                        //         auto new_ir = new ir::IR(ir::IR::OpKind::NEG, ir->opn2_, ir_next->res_);

                        //         *(iter + 1) = new_ir;
                        //     }else if(isSameOpn(ir_next->opn2_, ir->opn1_)){
                        //         auto new_ir = new ir::IR(ir::IR::OpKind::NEG, ir->opn1_, ir_next->res_);

                        //         *(iter + 1) = new_ir;
                        //     }                            
                        }                        
                    }                    
                }
                else if (ir::IR::OpKind::DIV == ir->op_){
                    if(ir::IR::OpKind::DIV == ir_next->op_){
                        if (isSameOpn(ir->res_, ir_next->opn1_)){
                            if (isImm(ir_next->opn2_)){
                                if(isImm(ir->opn1_)){
                                    int opn_num = ir->opn1_.imm_num_ / ir_next->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::DIV, new_opn, ir->opn2_, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }else if(isImm(ir->opn2_)){
                                    int opn_num = ir_next->opn2_.imm_num_ * ir->opn2_.imm_num_;
                                    auto new_opn = ir::Opn(ir::Opn::Type::Imm, opn_num);
                                    auto new_ir = new ir::IR(ir::IR::OpKind::DIV, ir->opn1_, new_opn, ir_next->res_);

                                    *(iter + 1) = new_ir;
                                    // bb->ir_list_.erase(iter + 1);
                                }
                            }
                        }
                    }else if(ir::IR::OpKind::MUL == ir_next->op_){
                        if(isSameOpn(ir->res_, ir_next->opn1_)){
                            if(isSameOpn(ir->opn2_, ir_next->opn2_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn1_, ir_next->res_);

                                *(iter + 1) = new_ir;
                            }
                        }else if(isSameOpn(ir->res_, ir_next->opn2_)){
                            if(isSameOpn(ir->opn2_, ir_next->opn1_)){
                                auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn1_, ir_next->res_);

                                *(iter + 1) = new_ir;                                
                            }
                        }
                    }                    
                }
                else if (ir::IR::OpKind::NEG == ir->op_){
                    if(ir::IR::OpKind::NEG == ir_next->op_){
                        if(isSameOpn(ir->res_, ir_next->opn1_)){
                            auto new_ir = new ir::IR(ir::IR::OpKind::ASSIGN, ir->opn1_, ir_next->res_);

                            *(iter + 1) = new_ir;
                        }
                    }
                }
            }
        }
    }
}