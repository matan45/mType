namespace company {
string name = "TechCorp";

function getCompanyName(): string {
    return name;
}

namespace departments {
    namespace engineering {
        int teamSize = 15;
        
        function calculateBudget(int members): int {
            return members * 1000;
        }
        
        function getTeamSize(): int {
            return teamSize;
        }
        
        namespace frontend {
            string framework = "React";
            int developers = 5;
            
            function getDeveloperCount(): int {
                return developers;
            }
        }
        
        namespace backend {
            string language = "C++";
            int developers = 10;
            
            function getDeveloperCount(): int {
                return developers;
            }
            
            function getTotalTeamSize(): int {
                return developers + 5;  // frontend developers count
            }
        }
    }
    
    namespace marketing {
        int campaigns = 3;
        
        function calculateReach(int budget): int {
            return budget * 100;
        }
        
        function getCampaignCount(): int {
            return campaigns;
        }
    }
}

namespace products {
    string mainProduct = "Software";
    int versions = 12;
    
    function getVersionCount(): int {
        return versions;
    }
}
}

// Test deeply nested function calls
int budget = company::departments::engineering::calculateBudget(20);
print(budget);

int frontendDevs = company::departments::engineering::frontend::getDeveloperCount();
print(frontendDevs);

int backendDevs = company::departments::engineering::backend::getDeveloperCount();
print(backendDevs);

int totalDevs = company::departments::engineering::backend::getTotalTeamSize();
print(totalDevs);

int reach = company::departments::marketing::calculateReach(50);
print(reach);

int versions = company::products::getVersionCount();
print(versions);