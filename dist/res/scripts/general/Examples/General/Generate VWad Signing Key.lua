-- Generate a new ASCII-encoded Public/Private key pair and present it to the user
local privkey = Archives.VWad.CreatePrivateKey()
local output = "Private Key: " .. privkey .. "\n"

function AddOutputLine(line)
   output = output .. line .. "\n"
end

AddOutputLine("Public Key: " .. Archives.VWad.DerivePublicKey(privkey) .. "\n")
AddOutputLine("Paste the Private Key into vwad_private_key (under SLADE Advanced Settings) to sign all VWads with it when saving.\n")
AddOutputLine("Share ONLY the Public Key with others!")

UI.MessageBox("New VWad Signing Key", output)