open Ast.Statement.DeclareModule;

open Ast.Statement.Block;

open Ast.Literal;

open Ast.Type;

open Ast.Type.Function;

open Ast.Type.Function.Param;

open Ast.Statement.DeclareExportDeclaration;

open Ast.Statement.DeclareVariable;

open Ast.Statement.DeclareFunction;

module JsType = {
  type t =
    | Null
    | Number
    | Function (list t) t
    | Unknown
    | Unit
    | Any;
};

let string_of_id (loc: Loc.t, id: string) => id;

let rec type_annotation_to_jstype (annotation: option Ast.Type.annotation) =>
  switch annotation {
  | Some (_, (_, t)) => type_to_jstype t
  | None => JsType.Unknown
  }
and type_to_jstype =
  fun
  | Any => JsType.Any
  | Null => JsType.Null
  | Number => JsType.Number
  | Function f => function_type_to_jstype f
  | _ => JsType.Unknown
and function_type_to_jstype {params: (formal, rest), returnType: (_, rt)} => {
  let params =
    if (List.length formal > 0) {
      List.map
        (fun ((_, {typeAnnotation: (_, t)}): Ast.Type.Function.Param.t) => type_to_jstype t) formal
    } else {
      [JsType.Unit]
    };
  let return = type_to_jstype rt;
  JsType.Function params return
};

module JsDecl = {
  type t =
    | VarDecl string JsType.t
    | FuncDecl string JsType.t
    | ModuleDecl string (list t)
    | Unknown;
};

let declaration_to_jsdecl =
  fun
  | Variable (loc, {id, typeAnnotation}) =>
    JsDecl.VarDecl (string_of_id id) (type_annotation_to_jstype typeAnnotation)
  | Function (loc, {id, typeAnnotation}) =>
    JsDecl.FuncDecl (string_of_id id) (type_annotation_to_jstype (Some typeAnnotation))
  | _ => JsDecl.Unknown;

let rec statement_to_stack (loc, s) =>
  switch s {
  | Ast.Statement.DeclareExportDeclaration {declaration: Some declaration} =>
    declaration_to_jsdecl declaration
  | Ast.Statement.DeclareModule s => declare_module_to_jsdecl s
  | _ => JsDecl.Unknown
  }
and block_to_stack (loc, {body}) => List.map statement_to_stack body
and declare_module_to_jsdecl {id, body} =>
  switch id {
  | Literal (loc, {raw}) => JsDecl.ModuleDecl raw (block_to_stack body)
  | _ => JsDecl.Unknown
  };

let rec show_type =
  fun
  | JsType.Any => "any"
  | JsType.Unit => "unit"
  | JsType.Function params return =>
    String.concat " => " (List.map show_type params) ^ " => " ^ show_type return
  | JsType.Null => "null"
  | JsType.Number => "number"
  | JsType.Unknown => "??";

let rec show_decl =
  fun
  | JsDecl.ModuleDecl name decls =>
    "module " ^ name ^ " = {\n  " ^ String.concat "\n  " (List.map show_decl decls) ^ "\n}"
  | JsDecl.Unknown => "external ??"
  | JsDecl.FuncDecl name of_type =>
    "external " ^ name ^ " : " ^ show_type of_type ^ " = \"\" [@@bs.send];"
  | JsDecl.VarDecl name of_type =>
    "external " ^ name ^ " : " ^ show_type of_type ^ " = \"\" [@@bs.val];";
