open Core.Std
open Ir

let validate_prefix fin fout intermediate_pref =
  let assumptions_fname = intermediate_pref ^ ".assumptions.vf" in
  let lino_fname = intermediate_pref ^ ".lino.int" in
  let export_out_fname = intermediate_pref ^ ".export.stdout" in
  let verify_out_fname = intermediate_pref ^ ".verify.stdout" in
  let ir = Import.build_ir fin in
  Render.render_ir ir fout;
  match Verifier.verify_file fout verify_out_fname with
  | Verifier.Valid -> printf "Valid.\n"
  | Verifier.Invalid _ ->
    begin
      let vf_assumptions = Verifier.export_assumptions fout ir.export_point
          assumptions_fname lino_fname export_out_fname
      in
      let ir = Analysis.induce_symbolic_assignments ir vf_assumptions in
      Render.render_ir ir fout;
      match Verifier.verify_file fout verify_out_fname with
      | Verifier.Valid -> printf "\\/alid.\n"
      | Verifier.Invalid cause -> printf "Failed: %s\n" cause
    end

let () =
  validate_prefix Sys.argv.(1) Sys.argv.(2) Sys.argv.(3)
