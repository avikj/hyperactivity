module iscas85_c17_spec (input logic n1, n2, n3, n6, n7, output logic n22, n23);
  wire n10 = ~(n1 & n3);
  wire n11 = ~(n3 & n6);
  wire n16 = ~(n2 & n11);
  wire n19 = ~(n11 & n7);
  assign n22 = ~(n10 & n16);
  assign n23 = ~(n16 & n19);
endmodule

module iscas85_c17_net (input logic n1, n2, n3, n6, n7, output logic n22, n23);
  wire w0; assign w0 = ~(n1 & n3);
  wire w1; assign w1 = ~(n3 & n6);
  wire w2; assign w2 = ~(n2 & w1);
  wire w3; assign w3 = ~(w1 & n7);
  assign n22 = ~(w0 & w2);
  assign n23 = ~(w2 & w3);
endmodule

module epfl_adder8_spec (input logic [7:0] a, b, output logic [8:0] sum);
  assign sum = {1'b0, a} + {1'b0, b};
endmodule

module fa_nand9 (input logic a, b, cin, output logic s, cout);
  wire w0 = ~(a & b);
  wire w1 = ~(a & w0);
  wire w2 = ~(w0 & b);
  wire w3 = ~(w1 & w2);
  wire w4 = ~(w3 & cin);
  wire w5 = ~(w3 & w4);
  wire w6 = ~(w4 & cin);
  assign s    = ~(w5 & w6);
  assign cout = ~(w0 & w4);
endmodule

module epfl_adder8_nand (input logic [7:0] a, b, output logic [8:0] sum);
  wire c0, c1, c2, c3, c4, c5, c6, c7;
  fa_nand9 u0 (.a(a[0]), .b(b[0]), .cin(1'b0), .s(sum[0]), .cout(c0));
  fa_nand9 u1 (.a(a[1]), .b(b[1]), .cin(c0),   .s(sum[1]), .cout(c1));
  fa_nand9 u2 (.a(a[2]), .b(b[2]), .cin(c1),   .s(sum[2]), .cout(c2));
  fa_nand9 u3 (.a(a[3]), .b(b[3]), .cin(c2),   .s(sum[3]), .cout(c3));
  fa_nand9 u4 (.a(a[4]), .b(b[4]), .cin(c3),   .s(sum[4]), .cout(c4));
  fa_nand9 u5 (.a(a[5]), .b(b[5]), .cin(c4),   .s(sum[5]), .cout(c5));
  fa_nand9 u6 (.a(a[6]), .b(b[6]), .cin(c5),   .s(sum[6]), .cout(c6));
  fa_nand9 u7 (.a(a[7]), .b(b[7]), .cin(c6),   .s(sum[7]), .cout(c7));
  assign sum[8] = c7;
endmodule
