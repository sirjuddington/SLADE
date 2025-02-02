-- Get the ASCII-encoded public key for the provided ASCII-encoded private key

local privkey = UI.PromptString("Derive Public VWad Key", "Enter Private VWad Key", "")

UI.MessageBox("Public VWad Key For " .. privkey, Archives.VWad.DerivePublicKey(privkey))