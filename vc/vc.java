import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Scanner;
import java.util.Stack;

public class vc {
	static int g_conn_num, g_acc_num;
	/**
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException {
		
		String topfile = "/home/chinmay/gen_ws/vc/topfile", connfile = "/home/chinmay/gen_ws/vc/connfile", 
				rtfile = "/home/chinmay/gen_ws/vc/rtfile", ftfile = "/home/chinmay/gen_ws/vc/ftfile", pathsfile = "/home/chinmay/gen_ws/vc/pathsfile";
		char flag = 'd';
		boolean pessimistic = false;
		//Set options based on command line
		for(int i = 0; i < args.length; i++){
			if(args[i].startsWith("-")){
				switch(args[i]){
				case "-top":
					topfile = args[i + 1];
					break;
				case "-conn":
					connfile = args[i + 1];
					break;
				case "-rt":
					rtfile = args[i + 1];
					break;
				case "-ft":
					ftfile = args[i + 1];
					break;
				case "-path":
					pathsfile = args[i + 1];
					break;
				case "-flag":
					char[] farr = args[i+1].toCharArray();
					flag = farr[0];
					if(args[i+1].equals("degree")){
						flag = 'g';
					}
					break;
				case "-p":
					if(args[i+1].equals("1")) pessimistic = true;
					break;
				}
			}
		}
		
		Scanner topf_sc = new Scanner(new FileReader(topfile));
		int nodecount, edgecount;
		nodecount = topf_sc.nextInt();
		edgecount = topf_sc.nextInt();
		topf_sc.close();
		
		double[][][] gdata = get_graph(topfile, flag);
		double[][] graph = gdata[0];
		double[][] capacity = gdata[1];
		double[][] delay = gdata[2];
		
		int[][][][] paths = new int[nodecount][][][];
		double[][][][] cost_delays = new double[nodecount][][][];
		for(int i=0; i< nodecount; i++){
			paths[i] = new int[nodecount][][];
			cost_delays[i] = new double[nodecount][][];
			for(int m= 0; m<nodecount;m++){
				cost_delays[i][m] = new double[2][];
				cost_delays[i][m][0] = new double[2];
				cost_delays[i][m][1] = new double[2];
				cost_delays[i][m][0][0] = 0;
				cost_delays[i][m][0][1] = 0;
				cost_delays[i][m][1][0] = 0;
				cost_delays[i][m][1][1] = 0;
				paths[i][m] = get_paths(graph,i,m, delay, cost_delays[i][m]);
			}
		}
		
		make_rtfile(rtfile,paths,cost_delays);
		
		int[][][] vcid;
		int[] conn_src, conn_dest, path_pri;
		Scanner connf_sc = new Scanner(new FileReader(connfile));
		int conn_num = connf_sc.nextInt();
		connf_sc.close();
		conn_src = new int[conn_num];
		conn_dest = new int[conn_num];
		path_pri = new int[conn_num];
		vcid = get_conn(connfile, graph, capacity, paths, pessimistic, conn_src, conn_dest, path_pri);
		make_paths_file(pathsfile, paths, vcid, conn_src, conn_dest, path_pri);
		
		make_forwd_file(ftfile, paths, vcid, conn_src, conn_dest, path_pri, nodecount);
		
	}
	
	static void make_rtfile(String rtfile, int[][][][] paths, double[][][][] cost_delays) throws IOException{
		FileWriter wr = new FileWriter(rtfile);
		int node_num = paths.length;
		for(int i = 0; i < node_num; i++){
		 	wr.write("Node "+ i + "\n");
			for(int m = 0; m < node_num; m++){
				if(i!=m && paths[i][m][0] != null){
					wr.write("Dest "+ m + "\npath: ");
					int[] a_path = paths[i][m][0];
					for(int node:a_path){
						wr.write(node + " ");
					}
					wr.write("cost: " + cost_delays[i][m][0][0]);
					wr.write(" delay: " + cost_delays[i][m][0][1]);
					wr.write("\n");
				}
				if(i!=m && paths[i][m][1] != null){
					wr.write("Dest "+ m + " Path 2\n");
					int[] a_path = paths[i][m][1];
					for(int node:a_path){
						wr.write(node + " ");
					}
					wr.write("cost: " + cost_delays[i][m][1][0]);
					wr.write(" delay: " + cost_delays[i][m][1][1]);
					wr.write("\n");
				}
			}
		}
		
		wr.close();
	}
	
	
	static void make_forwd_file(String fwdfile, int[][][][] paths ,int[][][] vcid, 
			int[] conn_src, int[] conn_dest, int[] path_pri, int node_num) throws IOException{
		int conn_num = g_acc_num;
		String[] table = new String[node_num];
		for(int i=0; i<node_num; i++){
			table[i] = "";
		}
		for(int i=0; i<conn_num; i++){
			int src = conn_src[i], dest = conn_dest[i];
			int[] cpath = paths[src][dest][path_pri[i]];
			for(int m=0; m<cpath.length-1;m++){
				int curr = cp_wrap(cpath, m, src);
				table[curr] = table[curr].concat(cp_wrap(cpath, m -1, src)+" ");
				table[curr] = table[curr].concat(vcid[ cp_wrap(cpath, m -1, src) ][curr][i] + " ");
				table[curr] = table[curr].concat(cpath[m+1] + " ");
				table[curr] = table[curr].concat(vcid[curr][cpath[m+1]][i] + "\n");
			}
		}
		FileWriter wr = new FileWriter(fwdfile);
		for(int i=0; i<node_num; i++){
			wr.write("Node " + i + "\n");
			wr.write(table[i]);
		}
		wr.close();
	}
	
	static int cp_wrap(int[] cpath,int m, int src){
		if(m == -1) return src;
		else return cpath[m];
	}
	
	static void make_paths_file(String pathsfile, int[][][][] paths ,int[][][] vcid, 
			int[] conn_src, int[] conn_dest, int[] path_pri) throws IOException{
		int conn_num = g_acc_num;
		FileWriter outfile = new FileWriter(pathsfile);
		outfile.write(g_conn_num + " " + g_acc_num);
		for(int i=0; i<conn_num; i++){
			int src = conn_src[i], dest = conn_dest[i];
			outfile.write(i + " s: " + src + " d: " + dest + " p: ");
			int[] cpath = paths[src][dest][path_pri[i]];
			for(int m = 0; m < cpath.length; m++){
				outfile.write(cpath[m] + " ");
			}
			
			outfile.write(" v: ");
			int curr = src;
			for(int m = 0; m < cpath.length; m++){
				outfile.write(vcid[curr][cpath[m]][i] + " ");
				curr = cpath[m];
			}
			outfile.write('\n');
		}
		
		outfile.close();
	}
	
	static int[][][] get_conn(String connfile, double[][] graph, double[][] capacity,int[][][][] paths, 
			boolean pessi, int[] conn_src, int[] conn_dest, int[] path_pri) throws IOException{
		int disallowed_num = 0;
		int nodecount = graph.length;
		Scanner sc_conn = new Scanner(new FileReader(connfile));
		int conn_num = sc_conn.nextInt();
		g_conn_num = conn_num;
		
		int[][][] connection = new int[nodecount][][];
		//[i][j][connection_id]
		double[][] used_cap = new double[nodecount][];
		int[][] used_link_id = new int[nodecount][];
		for(int i = 0; i<nodecount; i++){
			connection[i] = new int[nodecount][];
			used_cap[i] = new double[nodecount];
			used_link_id[i] = new int[nodecount];
			for(int m=0; m<nodecount; m++){
				connection[i][m] = new int[conn_num];
				for(int l=0;l<conn_num;l++){
					connection[i][m][l] = -1;
				}
			}
		}
		int connection_id = 0;
		while(sc_conn.hasNext()){
			int src,dest,min,avg,max;
			double equiv;
			src = sc_conn.nextInt();
			dest = sc_conn.nextInt();
			min = sc_conn.nextInt();
			avg = sc_conn.nextInt();
			max = sc_conn.nextInt();
			
			conn_src[connection_id] = src;
			conn_dest[connection_id] = dest;
			
			equiv = get_equiv(min, avg, max, pessi);
			int[] cpath = paths[src][dest][0];
			boolean gotpath = true;
			path_pri[connection_id] = 0;
			if(!check_allowed(capacity, used_cap, src, dest, cpath, equiv)){
				path_pri[connection_id] = 1;
				cpath = paths[src][dest][1];
				if(!check_allowed(capacity, used_cap, src, dest, cpath, equiv)){
					disallowed_num++;
					path_pri[connection_id] = -1;
					gotpath = false;
				}
			}
			
			if(gotpath){
				int curr = src;
				int i = 0;
				while(curr != dest){
					int next = cpath[i];
					used_cap[curr][next] += equiv;
					connection[curr][next][connection_id] = used_link_id[curr][next];
					used_link_id[curr][next]++;
					curr = next;
					i++;
				}
				
				connection_id++;
			}
			
		}
		g_acc_num = g_conn_num - disallowed_num;
		sc_conn.close();
		return connection;
	}
	
	//For a given path, checks if a connection can be admitted
	static boolean check_allowed(double[][] capacity, double[][] used_cap, int src, int dest, int[] path, double equiv){
		if(path == null) return false;
		int curr = src;
		int i = 0;
		while(curr != dest){
			int next = path[i];
			if(equiv+used_cap[curr][next] > capacity[curr][next]) return false;
			curr = next;
			i++;
		}
		return true;
	}
	
	static double get_equiv(int min,int  avg,int  max,boolean pessi){
		if(pessi) return max;
		else{
			int e = avg + (max - min)/4;
			return min>e?min:e;
		}
	}
	
	static double[][][] get_graph(String path, char flag) throws IOException{
		int nodecount,edgecount;
		FileReader f = new FileReader(path);
		Scanner sc = new Scanner(f);
		nodecount = sc.nextInt();
		edgecount = sc.nextInt();
		double[][][] graph = new double[3][][];
		//0 is costs 1 is capacity 2 is delay
		graph[0] = new double[nodecount][];
		graph[1] = new double[nodecount][];
		graph[2] = new double[nodecount][];
		for(int i=0; i< nodecount; i++){
			graph[0][i] = new double[nodecount];
			graph[1][i] = new double[nodecount];
			graph[2][i] = new double[nodecount];
			for(int m = 0; m < nodecount; m++){
				graph[0][i][m]= -1;
			}
		}
		while(sc.hasNext()){
			int src,dest,delay,capacity;
			double reliability;
			src = sc.nextInt();
			dest = sc.nextInt();
			delay = sc.nextInt();
			capacity = sc.nextInt();
			graph[1][src][dest] = capacity;
			graph[2][src][dest] = delay;
			reliability = sc.nextDouble();
			switch(flag){
			case 'd':
				graph[0][src][dest] = delay;
				break;
			case 'h':
				graph[0][src][dest] = 1;
				break;
			case 'r':
				graph[0][src][dest] =  -Math.log(reliability);
				break;
			//TODO degree metric
				
			}
			
		}
		f.close();
		sc.close();
		return graph;
	}
	
	
	static int[][] get_paths(double[][] costs, int src, int dest, double[][] delay, double[][] cost_delay){
		int nodecount = costs.length;
		double[][] sec_costs = new double[nodecount][];
		for(int i = 0; i < costs.length; i++){
			sec_costs[i] = costs[i].clone();
		}
		double currcost[] = new double[nodecount];
		boolean closed[] = new boolean[nodecount];
		boolean is_inf[] = new boolean[nodecount];
		int parent[] = new int[nodecount];
		for(int i = 0; i < nodecount; i++){
			is_inf[i] = true;
			closed[i] = false;
			parent[i] = -1;
		}
		currcost[src] = 0;
		int curr_node = src;
		boolean failed = false;
		while(curr_node != dest){
			for(int i = 0; i < nodecount; i++){
				if(costs[curr_node][i] != -1){
					if(currcost[i] > currcost[curr_node] + costs[curr_node][i] || is_inf[i]){
						parent[i] = curr_node;
						currcost[i] = currcost[curr_node] + costs[curr_node][i];
						is_inf[i] = false;
					}
				}
			}
			closed[curr_node] = true;
			curr_node = get_min_loc(currcost,src,closed,is_inf);
			if(curr_node == -1){
				failed = true;
				break;
			}
		}
		int[][] soln = new int[2][];
		if(parent[dest] == -1 || failed){
			soln[0] = soln[1] = null;
			return soln;
		}
		int back_node = dest;
		Stack<Integer> st = new Stack<>();
		while(back_node != src){
			cost_delay[0][0] += costs[parent[back_node]][back_node];
			cost_delay[0][1] += delay[parent[back_node]][back_node];
			st.push(back_node);
			back_node = parent[back_node];
		}
		soln[0] = new int[st.size()];
		int j = 0;
		while(!st.isEmpty()){
			soln[0][j] = st.pop();
			j++;
		}
		int rem_node = src;
		for(int i = 0; i<soln[0].length; i++){
			sec_costs[rem_node][soln[0][i]] = -1;
			rem_node = soln[0][i];
		}
		//second time
		for(int i = 0; i < nodecount; i++){
			is_inf[i] = true;
			closed[i] = false;
			currcost[i] = 0;
			parent[i] = -1;
		}
		currcost[src] = 0;
		curr_node = src;
		failed = false;
		while(curr_node != dest){
			for(int i = 0; i < nodecount; i++){
				if(sec_costs[curr_node][i] > 0){
					if(currcost[i] > currcost[curr_node] + sec_costs[curr_node][i] || is_inf[i]){
						parent[i] = curr_node;
						currcost[i] = currcost[curr_node] + sec_costs[curr_node][i];
						is_inf[i] = false;
					}
				}
			}
			closed[curr_node] = true;
			curr_node = get_min_loc(currcost, src,closed,is_inf); 
			if(curr_node == -1){
				failed = true;
				break;
			}
		}
		if(parent[dest] == -1 || failed){
			soln[1] = null;
			return soln;
		}
		back_node = dest;
		st = new Stack<>();
		while(back_node != src){
			cost_delay[1][0] += costs[parent[back_node]][back_node];
			cost_delay[1][1] += delay[parent[back_node]][back_node];
			st.push(back_node);
			back_node = parent[back_node];
		}
		soln[1] = new int[st.size()];
		j = 0;
		while(!st.isEmpty()){
			soln[1][j] = st.pop();
			j++;
		}
		return soln;
	}
	
	static int get_min_loc(double[] cost, int src,boolean[] closed,boolean[] is_inf){
		double currmin = Double.MAX_VALUE;
		int minloc = -1;
		for(int i = 0; i < cost.length; i++){
			if((cost[i] <= currmin && !is_inf[i])&& !closed[i] && i!=src){
				currmin = cost[i];
				minloc = i;
			}

		}
		return minloc;
	}

}
