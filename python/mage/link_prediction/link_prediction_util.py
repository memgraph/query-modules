from platform import node
import torch
import torch.nn.functional as F
import numpy as np
import dgl
from collections import defaultdict
from sklearn.metrics import roc_auc_score
from sklearn.metrics import accuracy_score
from sklearn.metrics import precision_score
from sklearn.metrics import recall_score
from sklearn.metrics import f1_score
from typing import Callable, Dict, Tuple, List
import mgp
import random
from mage.link_prediction.constants import (
    MODEL_NAME, NEG_PRED_EXAMPLES, POS_PRED_EXAMPLES, PREDICTOR_NAME, LOSS, ACCURACY, AUC_SCORE, PRECISION, RECALL, F1, EPOCH,
    POS_EXAMPLES, NEG_EXAMPLES
)

def get_number_of_edges(ctx: mgp.ProcCtx) -> int:
    """Returns number of edges for graph from execution context.

    Args:
        ctx (mgp.ProcCtx): A reference to the execution context.

    Returns:
        int: A number of edges.
    """
    edge_cnt = 0
    for vertex in ctx.graph.vertices:
        edge_cnt += len(list(vertex.out_edges))
    return edge_cnt

def preprocess(graph: dgl.graph, split_ratio: float, target_relation: str) -> Tuple[dgl.graph, dgl.graph, dgl.graph, dgl.graph, dgl.graph]:
    """Preprocess method splits dataset in training and validation set by creating necessary masks for distinguishing those two. 
        This method is also used for setting numpy and torch random seed.

    Args:
        graph (dgl.graph): A reference to the dgl graph representation.
        split_ratio (float): Split ratio training to validation set. E.g 0.8 indicates that 80% is used as training set and 20% for validation set.
        relation (Tuple[str, str, str]): [src_type, edge_type, dest_type] identifies edges on which model will be trained for prediction 

    Returns:
        Tuple[Dict[Tuple[str, str, str], List[int]], Dict[Tuple[str, str, str], List[int]]:
            1. Training mask: target relation to training edge IDs
            2. Validation mask: target relation to validation edge IDS
    """

    # First set all seeds
    rnd_seed = 0
    random.seed(rnd_seed)
    np.random.seed(rnd_seed)
    torch.manual_seed(rnd_seed)  # set it for both cpu and cuda

    # val size is 1-split_ratio specified by the user
    val_size = int(graph.number_of_edges() * (1 - split_ratio))

    # If user wants to split the dataset but it is too small, then raise an Exception
    if split_ratio < 1.0 and val_size == 0:
        raise Exception("Graph too small to have a validation dataset. ")
    
    # Get edge IDS
    edge_type_u, _ = graph.edges(etype=target_relation)
    graph_edges_len = len(edge_type_u)
    eids = np.arange(graph_edges_len)  # get all edge ids from number of edges and create a numpy vector from it.
    eids = np.random.permutation(eids)  # randomly permute edges

    # Get training and validation edges
    tr_eids, val_eids = eids[val_size:], eids[:val_size]

    # Create and masks that will be used in the batch training
    train_eid_dict, val_eid_dict = {target_relation: tr_eids}, {target_relation: val_eids}

    return train_eid_dict, val_eid_dict

def classify(probs: torch.tensor, threshold: float) -> torch.tensor:
    """Classifies based on probabilities of the class with the label one.

    Args:
        probs (torch.tensor): Edge probabilities.

    Returns:
        torch.tensor: classes
    """

    return probs > threshold

def evaluate(metrics: List[str], labels: torch.tensor, probs: torch.tensor, result: Dict[str, float], threshold: float, epoch: int, loss: float,
            operator: Callable[[float, float], float]) -> None:
    """Returns all metrics specified in metrics list based on labels and predicted classes. In-place modification of dictionary.

    Args:
        metrics (List[str]): List of string metrics.
        labels (torch.tensor): Predefined labels.
        probs (torch.tensor): Probabilities of the class with the label one.
        result (Dict[str, float]): Prepopulated result that needs to be modified.
        threshold (float): classification threshold. 0.5 for sigmoid etc.
    Returns:
        Dict[str, float]: Metrics embedded in dictionary -> name-value shape
    """
    classes = classify(probs, threshold)
    result[EPOCH] = epoch
    result[LOSS] = operator(result[LOSS], loss)
    for metric_name in metrics:
        if metric_name == ACCURACY:
            result[ACCURACY] = operator(result[ACCURACY], accuracy_score(labels, classes))
        elif metric_name == AUC_SCORE:
            result[AUC_SCORE] = operator(result[AUC_SCORE], roc_auc_score(labels, probs.detach()))
        elif metric_name == F1:
            result[F1] = operator(result[F1], f1_score(labels, classes))
        elif metric_name == PRECISION:
            result[PRECISION] = operator(result[PRECISION], precision_score(labels, classes))
        elif metric_name == RECALL:
            result[RECALL] = operator(result[RECALL], recall_score(labels, classes))
        elif metric_name == POS_PRED_EXAMPLES:
            result[POS_PRED_EXAMPLES] = operator(result[POS_PRED_EXAMPLES], (probs > 0.5).sum().item())
        elif metric_name == NEG_PRED_EXAMPLES:
            result[NEG_PRED_EXAMPLES] = operator(result[NEG_PRED_EXAMPLES], (probs < 0.5).sum().item())
        elif metric_name == POS_EXAMPLES:
            result[POS_EXAMPLES] = operator(result[POS_EXAMPLES], (labels == 1).sum().item())
        elif metric_name == NEG_EXAMPLES:
            result[NEG_EXAMPLES] = operator(result[NEG_EXAMPLES], (labels == 0).sum().item())

def batch_forward_pass(model: torch.nn.Module, predictor: torch.nn.Module, loss: torch.nn.Module, m: torch.nn.Module, 
                    target_relation: str, input_features: Dict[str, torch.Tensor], pos_graph: dgl.graph, 
                    neg_graph: dgl.graph, blocks: List[dgl.graph]) -> Tuple[torch.Tensor, torch.Tensor, torch.nn.Module]:
    """Performs one forward batch pass

    Args:
        model (torch.nn.Module): A reference to the model that needs to be trained. 
        predictor (torch.nn.Module): A reference to the edge predictor.
        loss (torch.nn.Module): Loss function.
        m (torch.nn.Module): The activation function.
        target_relation: str -> Unique edge type that is used for training.
        input_features (Dict[str, torch.Tensor]): A reference to the input_features that are needed to compute representations for second block.
        pos_graph (dgl.graph): A reference to the positive graph. All edges that should be included.
        neg_graph (dgl.graph): A reference to the negative graph. All edges that shouldn't be included.
        blocks (List[dgl.graph]): First DGLBlock(MFG) is equivalent to all necessary nodes that are needed to compute final representation.
            Second DGLBlock(MFG) is a mini-batch.

    Returns:
         Tuple[torch.Tensor, torch.Tensor, torch.nn.Module]: First tensor are calculated probabilities, second tensor are true labels and the last tensor
            is a reference to the loss.
    """

    outputs = model.forward(blocks, input_features)

    # Deal with edge scores
    pos_score = predictor.forward(pos_graph, outputs, target_relation=target_relation)
    neg_score = predictor.forward(neg_graph, outputs, target_relation=target_relation)
    scores = torch.cat([pos_score, neg_score])  # concatenated positive and negative score
    # probabilities
    probs = m(scores)
    labels = torch.cat([torch.ones(pos_score.shape[0]), torch.zeros(neg_score.shape[0])])  # concatenation of labels
    loss_output = loss(probs, labels)

    return probs, labels, loss_output

def inner_train(graph: dgl.graph,
                    train_eid_dict,
                    val_eid_dict, 
                    target_relation: str,
                    model: torch.nn.Module, 
                    predictor: torch.nn.Module, 
                    optimizer: torch.optim.Optimizer,
                    num_epochs: int,
                    node_features_property: str, 
                    console_log_freq: int, 
                    checkpoint_freq: int, 
                    metrics: List[str], 
                    tr_acc_patience: int, 
                    context_save_dir: str,
                    num_neg_per_pos_edge: int,
                    num_layers: int,
                    batch_size: int,
                    sampling_workers: int
                    ) -> Tuple[List[Dict[str, float]], torch.nn.Module, torch.Tensor]:
    """Batch training method. 

    Args:
        graph (dgl.graph): A reference to the original graph. 
        train_eid_dict (_type_): Mask that identifies training part of the graph. This included only edges from a given relation.
        val_eid_dict (_type_): Mask that identifies validation part of the graph. This included only edges from a given relation.
        target_relation: str -> Unique edge type that is used for training.
        model (torch.nn.Module): A reference to the model that will be trained.
        predictor (torch.nn.Module): A reference to the edge predictor.
        optimizer (torch.optim.Optimizer): A reference to the training optimizer.
        num_epochs (int): number of epochs for model training.
        node_features_property: (str): property name where the node features are saved.
        console_log_freq (int): How often results will be printed. All results that are printed in the terminal will be returned to the client calling Memgraph.
        checkpoint_freq (int): Select the number of epochs on which the model will be saved. The model is persisted on the disc.
        metrics (List[str]): Metrics used to evaluate model in training on the validation set.
            Epoch will always be displayed, you can add loss, accuracy, precision, recall, specificity, F1, auc_score etc.
        tr_acc_patience (int): Training patience, for how many epoch will accuracy drop on validation set be tolerated before stopping the training.
        context_save_dir (str): Path where the model and predictor will be saved every checkpoint_freq epochs.
        num_neg_per_pos_edge (int): Number of negative edges that will be sampled per one positive edge in the mini-batch.
        num_layers (int): Number of layers in the GNN architecture.
        batch_size (int): Batch size used in both training and validation procedure.
        sampling_workers (int): Number of workers that will cooperate in the sampling procedure in the training and validation.
    Returns:
        Tuple[List[Dict[str, float]], torch.nn.Module, torch.Tensor]: Training and validation results. _
    """
    # Define what will be returned
    training_results, validation_results = [], []

    # First define all necessary samplers
    negative_sampler = dgl.dataloading.negative_sampler.Uniform(k=num_neg_per_pos_edge)
    sampler = dgl.dataloading.MultiLayerFullNeighborSampler(num_layers=num_layers)  # gather messages from all node neighbors
    sampler = dgl.dataloading.as_edge_prediction_sampler(sampler, negative_sampler=negative_sampler, exclude="self")  # TODO: Add "self" and change that to reverse edges sometime
    
    # Define training and validation dictionaries
    # For heterogeneous full neighbor sampling we need to define a dictionary of edge types and edge ID tensors instead of a dictionary of node types and node ID tensors
    # DataLoader iterates over a set of edges in mini-batches, yielding the subgraph induced by the edge mini-batch and message flow graphs (MFGs) to be consumed by the module below.
    # first MFG, which is identical to all the necessary nodes needed for computing the final representations
    # Feed the list of MFGs and the input node features to the multilayer GNN and get the outputs.

    # Define training EdgeDataLoader
    train_dataloader = dgl.dataloading.DataLoader(
        graph,                                  # The graph
        train_eid_dict,  # The edges to iterate over
        sampler,                                # The neighbor sampler
        batch_size=batch_size,    # Batch size
        shuffle=True,       # Whether to shuffle the nodes for every epoch
        drop_last=False,    # Whether to drop the last incomplete batch
        num_workers=sampling_workers       # Number of sampler processes
    )

    # Define validation EdgeDataLoader
    validation_dataloader = dgl.dataloading.DataLoader(
        graph,                                  # The graph
        val_eid_dict,  # The edges to iterate over
        sampler,                                # The neighbor sampler
        batch_size=batch_size,    # Batch size
        shuffle=True,       # Whether to shuffle the nodes for every epoch
        drop_last=False,    # Whether to drop the last incomplete batch
        num_workers=sampling_workers       # Number of sampler processes
    )

    # Initialize loss
    loss = torch.nn.BCELoss()

    # Initialize activation func
    m, threshold = torch.nn.Sigmoid(), 0.5

    # Define lambda functions for operating on dictionaries
    add_: Callable[[float, float], float] = lambda prior, later: prior + later
    avg_: Callable[[float, float], float] = lambda prior, size: prior/size
    format_float: Callable[[float], float] = lambda prior: round(prior, 2)

    # Training
    max_val_acc, num_val_acc_drop = (-1.0, 0,)  # last maximal accuracy and number of epochs it is dropping

    # Iterate for every epoch
    for epoch in range(1, num_epochs+1):
        # Evaluation epoch
        if epoch % console_log_freq == 0:
            print("Epoch: ", epoch)
            epoch_training_result = defaultdict(float)
            epoch_validation_result = defaultdict(float)
        
        # Training batch
        num_batches = 0
        model.train()
        for _, pos_graph, neg_graph, blocks in train_dataloader:
            # Get input features needed to compute representations of second block
            input_features = blocks[0].ndata[node_features_property]
            # Perform forward pass
            probs, labels, loss_output = batch_forward_pass(model, predictor, loss, m, target_relation, input_features, pos_graph, neg_graph, blocks)

            # Make an optimization step
            optimizer.zero_grad()
            loss_output.backward()
            optimizer.step()

            # Evaluate on training set
            if epoch % console_log_freq == 0:
                evaluate(metrics, labels, probs, epoch_training_result, threshold, epoch, loss_output.item(), add_)

            # Increment num batches
            num_batches +=1 

        # Edit train results and evaluate on validation set
        if epoch % console_log_freq == 0:
            epoch_training_result = {key: format_float(avg_(val, num_batches)) if key != EPOCH else val for key, val in epoch_training_result.items()}
            training_results.append(epoch_training_result)

            # Evaluate on the validation set
            model.eval()
            with torch.no_grad():
                num_batches = 0
                for _, pos_graph, neg_graph, blocks in validation_dataloader:
                    input_features = blocks[0].ndata[node_features_property]
                    # Perform forward pass
                    probs, labels, loss_output = batch_forward_pass(model, predictor, loss, m, target_relation, input_features, pos_graph, neg_graph, blocks)

                    # Add to the epoch_validation_result for saving
                    evaluate(metrics, labels, probs, epoch_validation_result, threshold, epoch, loss_output.item(), add_)
                    num_batches += 1

            if num_batches > 0: # Because it is possible that user specified not to have a validation dataset
                # Average over batches    
                epoch_validation_result = {key: format_float(avg_(val, num_batches)) if key != EPOCH else val for key, val in epoch_validation_result.items()}
                validation_results.append(epoch_validation_result)

                if ACCURACY in metrics:  # If user doesn't want to have accuracy information, it cannot be checked for patience.
                    # Patience check
                    if epoch_validation_result[ACCURACY] <= max_val_acc:
                        num_val_acc_drop += 1
                    else:
                        max_val_acc = epoch_validation_result[ACCURACY]
                        num_val_acc_drop = 0

                    # Stop the training if necessary
                    if num_val_acc_drop == tr_acc_patience:
                        print("Stopped because of validation criteria. ")
                        break

        # Save the model if necessary
        if epoch % checkpoint_freq == 0:
            _save_context(model, predictor, context_save_dir)

    # Save model at the end of the training
    _save_context(model, predictor, context_save_dir)

    return training_results, validation_results

def _save_context(model: torch.nn.Module, predictor: torch.nn.Module, context_save_dir: str):
    """Saves model and predictor to path.

    Args:
        context_save_dir: str -> Path where the model and predictor will be saved every checkpoint_freq epochs.
        model (torch.nn.Module): A reference to the model.
        predictor (torch.nn.Module): A reference to the predictor.
    """
    torch.save(model, context_save_dir + MODEL_NAME)
    torch.save(predictor, context_save_dir + PREDICTOR_NAME)

def inner_predict(model: torch.nn.Module, predictor: torch.nn.Module, graph: dgl.graph, node_features_property: str, 
                    src_node: int, dest_node: int, target_relation: str=None) -> float:
    """Predicts edge scores for given graph. This method is called to obtain edge probability for edge with id=edge_id.

    Args:
        model (torch.nn.Module): A reference to the trained model.
        predictor (torch.nn.Module): A reference to the predictor.
        graph (dgl.graph): A reference to the graph. This is semi-inductive setting so new nodes are appended to the original graph(train+validation).
        node_features_property (str): Property name of the features.
        src_node (int): Source node of the edge.
        dest_node (int): Destination node of the edge. 
        target_relation: str -> Unique edge type that is used for training.

    Returns:
        float: Edge score.
    """
    # TODO: CHANEG
    graph_features = {node_type: graph.nodes[node_type].data[node_features_property] for node_type in graph.ntypes}
    print("Target relation: ", target_relation)
    with torch.no_grad():
        h = model.forward_util(graph, graph_features)
        print("H: ", h)
        if type(target_relation) == tuple:  # Tuple[str, str, str] identification
            src_embedding, dest_embedding = h[target_relation[0]][src_node], h[target_relation[2]][dest_node]
        elif type(target_relation) == str:  # Edge type identification
            for key, val in h.items():
                if key[1] == target_relation:            
                    src_embedding, dest_embedding = val[src_node], val[dest_node]
        
        score = predictor.forward_pred(src_embedding, dest_embedding)
        # print("Scores: ", torch.sum(scores < 0.5).item())
        prob = torch.sigmoid(score)
        # print("Probability: ", prob.item())
        return prob.item()
        
# Existing
# 591017->49847
# 49847->2440
# 591017->2440
# 1128856->75969
# 31336->31349
# non-existing
# 31336->1106406
# 31336->37879
# 31336->1126012
# 31336->1107140
# 31336->1102850
# 31336->1106148
# 31336->1123188
# 31336->1128990

# Telecom recommendations
# 8779-QRDMV
# 7495-OOKFY

