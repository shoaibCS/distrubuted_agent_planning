#include "agent.h"

using namespace agent;


void Agent::networkModel() {

}

void Agent::PlanMsgCallback(const distributed_mapf::PathMsg& msg) {
	// OLEG TODO: move the following if's contect to networkModel function,
	//            so that we can use the same network simulation for all topics.
	if (agent_id_ != (unsigned int) msg.sender_id) {
		
		cout << "Got msg" << endl;
		// Drop the packet if the random number is not multiple of ten

		srand (time(NULL));
		int first_random_num = rand() % 100 + 1;
		
		if(first_random_num%10!=0){
			// Delay the packet if the random number is multple of 3.

			int second_random_num = rand() % 90 + 1;

			if (second_random_num%3==0)
				sleep(2);
			
			ROS_INFO("Agent [%s]:I heard a plan message from agent [%s], change command [%s]",
				std::to_string(agent_id_).c_str(), 
				std::to_string(msg.sender_id).c_str(),
				std::to_string(msg.set_new_plan).c_str());

			if(!msg.set_new_plan) {
				//message from agent A to B notifying of A' change of plan.
				list<planning::GraphIndex> recievedPlan;
				ConvertPathMsgToGraphIndexList(msg, recievedPlan);
				
				if (DetectCollision(recievedPlan)) {
					JointReplan(recievedPlan, msg.sender_id);
				

					for (unsigned int i = 0; mapf_Astar_.NumOfAgents(); i++) {
						if (mapf_Astar_.GetAgentIdFromIndex(i) == agent_id_)
							continue;

						distributed_mapf::PathMsg reply_msg;
			      		reply_msg.sender_id = agent_id_;
		    	  		reply_msg.target_id = mapf_Astar_.GetAgentIdFromIndex(i); 
		    	  		reply_msg.set_new_plan = true;
		    	  		ConvertGraphIndexListToPathMsg(mapf_Astar_.GetAgentPath(i), reply_msg);
		    	  		PublishPlan(reply_msg);
					}
				} 
			} else {
				   // message B from A after B detected a collision and recalculated a joint plan
				   // for both B and A
				   ChangePlan(msg);

				
			}
		}
	}
}

void Agent::ChangePlan(const distributed_mapf::PathMsg& msg) {
	cout << "changing plan: " << endl;
	list<planning::GraphIndex> new_plan;
	ConvertPathMsgToGraphIndexList(msg, new_plan);
	my_plan_ = new_plan;
}


void Agent::JointReplan(const list<planning::GraphIndex>& recieved_plan, unsigned int other_agent_id) {
	cout << "Agent::" << agent_id_ << "::Starting Joint Replan..." << endl;
	
	mapf_graph_.Init();
    mapf_graph_.AddAgentGraph(graph_); // OLEG TODO: Need to "redistrict it" 
    								   // aroud the collision point
    mapf_graph_.AddAgentToJointSpace(agent_id_, GetStartVertex(),
                                    GetGoalVertex());
    mapf_graph_.AddAgentToJointSpace(other_agent_id, recieved_plan.front(), 
    								recieved_plan.back());
    mapf_graph_.MergeAgentGraphs();

    mapf_Astar_ = planning::MultiAgentAstar(mapf_graph_, 1, 0.5);
    mapf_Astar_.GeneratePath();

}





