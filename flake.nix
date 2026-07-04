{
	description = "flake for chip-8 emulation";

		inputs = {
		nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
	};

	outputs = {  nixpkgs, ...}: let
	system = "x86_64-linux";
	pkgs = import nixpkgs { inherit system; };
	in {
		devShells."${system}".default = pkgs.mkShell {
		  packages = with pkgs; [
		  sdl3.lib
			sdl3.dev
		  clang

		
		  ];
        #	shellHook = '' 
	#		trap "nix-collect-garbage" EXIT
	#		echo "Garbage collection scheduled for exit."
	#	'';
		};
	};
}
